// ============================================================================
//
// = LIBRARY
//    ULib - c++ library
//
// = FILENAME
//    orm.h - Object Relational Mapping
//
// = AUTHOR
//    Stefano Casazza
//
// ============================================================================

#ifndef ULIB_ORM_H
#define ULIB_ORM_H 1

#include <ulib/string.h>

class UOrmDriver;
class UOrmStatement;
class USqlStatement;

/**
 * \brief SQL session object that represents a single connection and is the gateway to SQL database
 *
 * It is the main class that is used for access to the DB
 */

class U_EXPORT UOrmSession {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UOrmSession(const char* dbname,  uint32_t len);
   UOrmSession(const char* backend, uint32_t len, const UString& option)
      {
      U_TRACE_CTOR(0, UOrmSession, "%.*S,%u,%V", len, backend, len, option.rep)

      loadDriver(backend, len, option);
      }

   ~UOrmSession();

   // will be typecast into conn-specific type

   bool  isReady() const __pure;
   void* getConnection() __pure;
   bool  connect(const UString& option);

   UOrmDriver* getDriver() const { return pdrv; }

   // statement that should only be executed once and immediately

   bool query(const char* query, uint32_t query_len);

   // This function returns the number of database rows that were changed
   // or inserted or deleted by the most recently completed SQL statement

   unsigned long long affected();

   // This routine returns the rowid of the most recent successful INSERT into the database

   unsigned long long last_insert_rowid(const char* sequence = U_NULLPTR);

   // STREAM

#ifdef U_STDCPP_ENABLE
   friend U_EXPORT bool operator<<(UOrmSession& session, const char*    query) { return session.query(query, u__strlen(query, __PRETTY_FUNCTION__)); }
   friend U_EXPORT bool operator<<(UOrmSession& session, const UString& query) { return session.query(U_STRING_TO_PARAM(query)); }

   // DEBUG

#  ifdef DEBUG
   const char* dump(bool reset) const;
#  endif
#endif

protected:
   UOrmDriver* pdrv;

   void loadDriver(const char* backend, uint32_t len, const UString& option);

private:
   static void loadDriverFail(const char* ptr, uint32_t len) __noreturn U_NO_EXPORT;

   U_DISALLOW_COPY_AND_ASSIGN(UOrmSession)

   friend class UOrmStatement;
};

class U_EXPORT UOrmTypeHandler_Base {
public:
   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   UOrmTypeHandler_Base(const void* ptr) : pval((void*)ptr)
      {
      U_TRACE_CTOR(0, UOrmTypeHandler_Base, "%p", ptr)

      U_INTERNAL_ASSERT_POINTER(pval)
      }

   ~UOrmTypeHandler_Base()
      {
      U_TRACE_DTOR(0, UOrmTypeHandler_Base)

      U_INTERNAL_ASSERT_POINTER(pval)
      }

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
   const char* dump(bool reset) const;
#endif

protected:
   void* pval;

private:
   U_DISALLOW_ASSIGN(UOrmTypeHandler_Base)
};

#define U_ORM_TYPE_HANDLER(name_object_member, type_object_member) UOrmTypeHandler<type_object_member>(name_object_member)

/**
 * Converts Rows to a Type and the other way around.
 * Provide template specializations to support your own complex types.
 *
 * Take as example the following (simplified) class:
 *
 * class Person {
 * public:
 *    int     age;
 *    UString  lastName;
 *    UString firstName;
 * };
 *
 * add this methods to the (simplified) class:
 * 
 * void bindParam(UOrmStatement* stmt)
 *    {
 *    // the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Age INTEGER(3))
 *
 *    stmt->bindParam(U_ORM_TYPE_HANDLER(age,       int));
 *    stmt->bindParam(U_ORM_TYPE_HANDLER( lastName, UString));
 *    stmt->bindParam(U_ORM_TYPE_HANDLER(firstName, UString));
 *    }
 * 
 * void bindResult(UOrmStatement* stmt)
 *    {
 *    // the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Age INTEGER(3))
 *
 *    stmt->bindResult(U_ORM_TYPE_HANDLER(age,       int));
 *    stmt->bindResult(U_ORM_TYPE_HANDLER( lastName, UString));
 *    stmt->bindResult(U_ORM_TYPE_HANDLER(firstName, UString));
 *    }
 */

template <class T> class U_EXPORT UOrmTypeHandler : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(T* val) : UOrmTypeHandler_Base( val) {}
   explicit UOrmTypeHandler(T& val) : UOrmTypeHandler_Base(&val) {}

   // SERVICES

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<T>::bindParam(%p)", stmt)

      ((T*)pval)->bindParam(stmt);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<T>::bindResult(%p)", stmt)

      ((T*)pval)->bindResult(stmt);
      }

private:
   U_DISALLOW_ASSIGN(UOrmTypeHandler)
};

/**
 * \brief This class represents a prepared (not ordinary) statement that can be executed
 *
 * Placeholders will escape "special" characters for you automatically, protect you from
 * SQL injection vulnerabilities, and potentially make your code faster and cleaner to read.
 * The character '?' is used as placeholders in prepared statements
 */

class U_EXPORT UOrmStatement {
public:

   // Check for memory error
   U_MEMORY_TEST

   // Allocator e Deallocator
   U_MEMORY_ALLOCATOR
   U_MEMORY_DEALLOCATOR

   // The string must include one or more parameter markers in the SQL statement by embedding question mark (?) characters into
   // the SQL string at the appropriate positions. The markers are legal only in certain places in SQL statements. For example,
   // they are permitted in the VALUES() list of an INSERT statement (to specify column values for a row), or in a comparison with
   // a column in a WHERE clause to specify a comparison value.

    UOrmStatement(UOrmSession& session, const char* query, uint32_t query_len);
   ~UOrmStatement();

   // will be typecast into conn-specific type

   UOrmDriver*    getDriver() const    { return pdrv; }
   USqlStatement* getStatement() const { return pstmt; }

   // Execute the statement

   void execute();

   // ASYNC with PIPELINE

   bool asyncPipelineProcessQueue(uint32_t n);
   bool asyncPipelineSendQueryPrepared(uint32_t i);
   bool asyncPipelineMode(vPFu function = U_NULLPTR);
   void setAsyncPipelineHandlerResult(vPFu function);
   bool asyncPipelineSendQuery(const char* query, uint32_t query_len, uint32_t n);

   // This function returns the number of database rows that were changed
   // or inserted or deleted by the most recently completed SQL statement

   unsigned long long affected();

   // This routine returns the rowid of the most recent successful INSERT into the database

   unsigned long long last_insert_rowid(const char* sequence = U_NULLPTR);

   // Get number of columns in the row

   unsigned int cols();

   // Move forward to next row, returns false if no more rows available

   bool nextRow();

   // Resets a prepared statement on client and server to state after creation

   void reset();

   // Syntactic sugar for bindParam() used with use() binding param registers

#if defined(HAVE_CXX17)
template <typename...Ts> void use(Ts&&... ts) { (bindParam(UOrmTypeHandler<Ts>(ts)), ...); }
#else
template <class T1> 
void use(T1& r1);

template <class T1,class T2> 
void use(T1& r1,T2& r2);

template <class T1,class T2,class T3> 
void use(T1& r1,T2& r2,T3& r3);

template <class T1,class T2,class T3,class T4> 
void use(T1& r1,T2& r2,T3& r3,T4& r4);

template <class T1,class T2,class T3,class T4,class T5> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5);

template <class T1,class T2,class T3,class T4,class T5,class T6> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19,class T20> 
void use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19,T20& r20);
#endif

   // BIND PARAM

   void bindParam();
   void bindParam(int& v);
   void bindParam(bool& v);
   void bindParam(char& v);
   void bindParam(long& v);
   void bindParam(short& v);
   void bindParam(float& v);
   void bindParam(double& v);
#ifdef U_STDCPP_ENABLE
   void bindParam(istream& v);
#endif
   void bindParam(long long& v);
   void bindParam(struct tm& v);
   void bindParam(const char* s);
   void bindParam(long double& v);
   void bindParam(unsigned int& v);
   void bindParam(unsigned char& v);
   void bindParam(unsigned long& v);
   void bindParam(unsigned short& v);
   void bindParam(unsigned long long& v);
   void bindParam(const char* b, const char* e);
   void bindParam(const char* s, uint32_t n, bool bstatic, int rebind = -1);

   void bindParam(UString& v);
   void bindParam(UStringRep& v);

   template <typename T> void bindParam(UOrmTypeHandler<T> t)
      {
      U_TRACE(0, "UOrmStatement::bindParam<T>(%p)", &t)

      t.bindParam(this);
      }

   template <typename T> void bindParam(UOrmTypeHandler<T>& t)
      {
      U_TRACE(0, "UOrmStatement::bindParam<T>(%p)", &t)

      t.bindParam(this);
      }

   /**
    * Syntactic sugar for bindResult() used with into() binding result registers
    *
    * you could replace all of this with a single variadic template method and a c++17 fold expression (Victor Stewart)
    */

#if defined(HAVE_CXX17)
template <typename...Ts> void into(Ts&&... ts) { (bindResult(UOrmTypeHandler<Ts>(ts)), ...); }
#else 
template <class T1> 
void into(T1& r1);

template <class T1,class T2> 
void into(T1& r1,T2& r2);

template <class T1,class T2,class T3> 
void into(T1& r1,T2& r2,T3& r3);

template <class T1,class T2,class T3,class T4> 
void into(T1& r1,T2& r2,T3& r3,T4& r4);

template <class T1,class T2,class T3,class T4,class T5> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5);

template <class T1,class T2,class T3,class T4,class T5,class T6> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19);

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19,class T20> 
void into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19,T20& r20);
#endif

   // BIND RESULT

   void bindResult(int& v);
   void bindResult(bool& v);
   void bindResult(char& v);
   void bindResult(long& v);
   void bindResult(short& v);
   void bindResult(float& v);
   void bindResult(double& v);
   void bindResult(long long& v);
   void bindResult(long double& v);
   void bindResult(unsigned int& v);
   void bindResult(unsigned long& v);
   void bindResult(unsigned char& v);
   void bindResult(unsigned short& v);
   void bindResult(unsigned long long& v);

   void bindResult(UString& v);

   template <typename T> void bindResult(UOrmTypeHandler<T> t)
      {
      U_TRACE(0, "UOrmStatement::bindResult<T>(%p)", &t)

      t.bindResult(this);
      }

   template <typename T> void bindResult(UOrmTypeHandler<T>& t)
      {
      U_TRACE(0, "UOrmStatement::bindResult<T>(%p)", &t)

      t.bindResult(this);
      }

   // DEBUG

#if defined(U_STDCPP_ENABLE) && defined(DEBUG)
   const char* dump(bool reset) const;
#endif

protected:
   UOrmDriver* pdrv;
   USqlStatement* pstmt;
   UOrmSession* psession;

private:
   U_DISALLOW_COPY_AND_ASSIGN(UOrmStatement)
};

// Syntactic sugar for bindParam() used with use() binding registers

#if !defined(HAVE_CXX17)
template <class T1>
inline void UOrmStatement::use(T1& r1)
{
   U_TRACE(0, "UOrmStatement::use<T1&>(%p)",&r1)

   bindParam(UOrmTypeHandler<T1>(r1));
}

template <class T1,class T2>
inline void UOrmStatement::use(T1& r1,T2& r2)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&>(%p,%p)",&r1,&r2)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
}

template <class T1,class T2,class T3>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&>(%p,%p,%p)",&r1,&r2,&r3)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
}

template <class T1,class T2,class T3,class T4>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&>(%p,%p,%p,%p)",&r1,&r2,&r3,&r4)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
}

template <class T1,class T2,class T3,class T4,class T5>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&>(%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
}

template <class T1,class T2,class T3,class T4,class T5,class T6>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&>(%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&>(%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&>(%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&>(%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
   bindParam(UOrmTypeHandler<T15>(r15));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
   bindParam(UOrmTypeHandler<T15>(r15));
   bindParam(UOrmTypeHandler<T16>(r16));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
   bindParam(UOrmTypeHandler<T15>(r15));
   bindParam(UOrmTypeHandler<T16>(r16));
   bindParam(UOrmTypeHandler<T17>(r17));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&,T18&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17,&r18)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
   bindParam(UOrmTypeHandler<T15>(r15));
   bindParam(UOrmTypeHandler<T16>(r16));
   bindParam(UOrmTypeHandler<T17>(r17));
   bindParam(UOrmTypeHandler<T18>(r18));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&,T18&,T19&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17,&r18,&r19)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
   bindParam(UOrmTypeHandler<T15>(r15));
   bindParam(UOrmTypeHandler<T16>(r16));
   bindParam(UOrmTypeHandler<T17>(r17));
   bindParam(UOrmTypeHandler<T18>(r18));
   bindParam(UOrmTypeHandler<T19>(r19));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19,class T20>
inline void UOrmStatement::use(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19,T20& r20)
{
   U_TRACE(0, "UOrmStatement::use<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&,T18&,T19&,T20&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17,&r18,&r19,&r20)

   bindParam(UOrmTypeHandler<T1>(r1));
   bindParam(UOrmTypeHandler<T2>(r2));
   bindParam(UOrmTypeHandler<T3>(r3));
   bindParam(UOrmTypeHandler<T4>(r4));
   bindParam(UOrmTypeHandler<T5>(r5));
   bindParam(UOrmTypeHandler<T6>(r6));
   bindParam(UOrmTypeHandler<T7>(r7));
   bindParam(UOrmTypeHandler<T8>(r8));
   bindParam(UOrmTypeHandler<T9>(r9));
   bindParam(UOrmTypeHandler<T10>(r10));
   bindParam(UOrmTypeHandler<T11>(r11));
   bindParam(UOrmTypeHandler<T12>(r12));
   bindParam(UOrmTypeHandler<T13>(r13));
   bindParam(UOrmTypeHandler<T14>(r14));
   bindParam(UOrmTypeHandler<T15>(r15));
   bindParam(UOrmTypeHandler<T16>(r16));
   bindParam(UOrmTypeHandler<T17>(r17));
   bindParam(UOrmTypeHandler<T18>(r18));
   bindParam(UOrmTypeHandler<T19>(r19));
   bindParam(UOrmTypeHandler<T20>(r20));
}
#endif

// Syntactic sugar for bindResult() used with into() binding result registers

#if !defined(HAVE_CXX17)
template <class T1>
inline void UOrmStatement::into(T1& r1)
{
   U_TRACE(0, "UOrmStatement::into<T1&>(%p)",&r1)

   bindResult(UOrmTypeHandler<T1>(r1));
}

template <class T1,class T2>
inline void UOrmStatement::into(T1& r1,T2& r2)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&>(%p,%p)",&r1,&r2)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
}

template <class T1,class T2,class T3>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&>(%p,%p,%p)",&r1,&r2,&r3)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
}

template <class T1,class T2,class T3,class T4>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&>(%p,%p,%p,%p)",&r1,&r2,&r3,&r4)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
}

template <class T1,class T2,class T3,class T4,class T5>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&>(%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
}

template <class T1,class T2,class T3,class T4,class T5,class T6>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&>(%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&>(%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&>(%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&>(%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
   bindResult(UOrmTypeHandler<T15>(r15));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
   bindResult(UOrmTypeHandler<T15>(r15));
   bindResult(UOrmTypeHandler<T16>(r16));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
   bindResult(UOrmTypeHandler<T15>(r15));
   bindResult(UOrmTypeHandler<T16>(r16));
   bindResult(UOrmTypeHandler<T17>(r17));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&,T18&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17,&r18)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
   bindResult(UOrmTypeHandler<T15>(r15));
   bindResult(UOrmTypeHandler<T16>(r16));
   bindResult(UOrmTypeHandler<T17>(r17));
   bindResult(UOrmTypeHandler<T18>(r18));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&,T18&,T19&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17,&r18,&r19)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
   bindResult(UOrmTypeHandler<T15>(r15));
   bindResult(UOrmTypeHandler<T16>(r16));
   bindResult(UOrmTypeHandler<T17>(r17));
   bindResult(UOrmTypeHandler<T18>(r18));
   bindResult(UOrmTypeHandler<T19>(r19));
}

template <class T1,class T2,class T3,class T4,class T5,class T6,class T7,class T8,class T9,class T10,class T11,class T12,class T13,class T14,class T15,class T16,class T17,class T18,class T19,class T20>
inline void UOrmStatement::into(T1& r1,T2& r2,T3& r3,T4& r4,T5& r5,T6& r6,T7& r7,T8& r8,T9& r9,T10& r10,T11& r11,T12& r12,T13& r13,T14& r14,T15& r15,T16& r16,T17& r17,T18& r18,T19& r19,T20& r20)
{
   U_TRACE(0, "UOrmStatement::into<T1&,T2&,T3&,T4&,T5&,T6&,T7&,T8&,T9&,T10&,T11&,T12&,T13&,T14&,T15&,T16&,T17&,T18&,T19&,T20&>(%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p,%p)",&r1,&r2,&r3,&r4,&r5,&r6,&r7,&r8,&r9,&r10,&r11,&r12,&r13,&r14,&r15,&r16,&r17,&r18,&r19,&r20)

   bindResult(UOrmTypeHandler<T1>(r1));
   bindResult(UOrmTypeHandler<T2>(r2));
   bindResult(UOrmTypeHandler<T3>(r3));
   bindResult(UOrmTypeHandler<T4>(r4));
   bindResult(UOrmTypeHandler<T5>(r5));
   bindResult(UOrmTypeHandler<T6>(r6));
   bindResult(UOrmTypeHandler<T7>(r7));
   bindResult(UOrmTypeHandler<T8>(r8));
   bindResult(UOrmTypeHandler<T9>(r9));
   bindResult(UOrmTypeHandler<T10>(r10));
   bindResult(UOrmTypeHandler<T11>(r11));
   bindResult(UOrmTypeHandler<T12>(r12));
   bindResult(UOrmTypeHandler<T13>(r13));
   bindResult(UOrmTypeHandler<T14>(r14));
   bindResult(UOrmTypeHandler<T15>(r15));
   bindResult(UOrmTypeHandler<T16>(r16));
   bindResult(UOrmTypeHandler<T17>(r17));
   bindResult(UOrmTypeHandler<T18>(r18));
   bindResult(UOrmTypeHandler<T19>(r19));
   bindResult(UOrmTypeHandler<T20>(r20));
}
#endif

// TEMPLATE SPECIALIZATIONS

template <> class U_EXPORT UOrmTypeHandler<null> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(null& val) : UOrmTypeHandler_Base(U_NULLPTR) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<null>::bindParam(%p)", stmt)

      stmt->bindParam();
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<null>::bindResult(%p)", stmt)
      }
};

template <> class U_EXPORT UOrmTypeHandler<bool> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(bool& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<bool>::bindParam(%p)", stmt)

      stmt->bindParam(*(bool*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<bool>::bindResult(%p)", stmt)

      stmt->bindResult(*(bool*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<char> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(char& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<char>::bindParam(%p)", stmt)

      stmt->bindParam(*(char*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<char>::bindResult(%p)", stmt)

      stmt->bindResult(*(char*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<unsigned char> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(unsigned char& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned char>::bindParam(%p)", stmt)

      stmt->bindParam(*(unsigned char*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned char>::bindResult(%p)", stmt)

      stmt->bindResult(*(unsigned char*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<short> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(short& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<short>::bindParam(%p)", stmt)

      stmt->bindParam(*(short*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<short>::bindResult(%p)", stmt)

      stmt->bindResult(*(short*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<unsigned short> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(unsigned short& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned short>::bindParam(%p)", stmt)

      stmt->bindParam(*(unsigned short*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned short>::bindResult(%p)", stmt)

      stmt->bindResult(*(unsigned short*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<int> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(int& val) : UOrmTypeHandler_Base(&val)
      {
      U_TRACE(0, "UOrmTypeHandler::UOrmTypeHandler<int>(%d)", val)

      U_INTERNAL_DUMP("this = %p", this)
      }

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<int>::bindParam(%p)", stmt)

      U_INTERNAL_DUMP("pval(%p) = %d", pval, *(int*)pval)

      stmt->bindParam(*(int*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<int>::bindResult(%p)", stmt)

      U_INTERNAL_DUMP("pval(%p) = %d", pval, *(int*)pval)

      stmt->bindResult(*(int*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<unsigned int> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(unsigned int& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned int>::bindParam(%p)", stmt)

      stmt->bindParam(*(unsigned int*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned int>::bindResult(%p)", stmt)

      stmt->bindResult(*(unsigned int*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<long> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(long& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<long>::bindParam(%p)", stmt)

      stmt->bindParam(*(long*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<long>::bindResult(%p)", stmt)

      stmt->bindResult(*(long*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<unsigned long> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(unsigned long& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned long>::bindParam(%p)", stmt)

      stmt->bindParam(*(unsigned long*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned long>::bindResult(%p)", stmt)

      stmt->bindResult(*(unsigned long*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<long long> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(long long& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<long long>::bindParam(%p)", stmt)

      stmt->bindParam(*(long long*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<long long>::bindResult(%p)", stmt)

      stmt->bindResult(*(long long*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<unsigned long long> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(unsigned long long& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned long long>::bindParam(%p)", stmt)

      stmt->bindParam(*(unsigned long long*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<unsigned long long>::bindResult(%p)", stmt)

      stmt->bindResult(*(unsigned long long*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<float> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(float& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<float>::bindParam(%p)", stmt)

      stmt->bindParam(*(float*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<float>::bindResult(%p)", stmt)

      stmt->bindResult(*(float*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<double> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(double& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<double>::bindParam(%p)", stmt)

      stmt->bindParam(*(double*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<double>::bindResult(%p)", stmt)

      stmt->bindResult(*(double*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<long double> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(long double& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<long double>::bindParam(%p)", stmt)

      stmt->bindParam(*(long double*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<long double>::bindResult(%p)", stmt)

      stmt->bindResult(*(long double*)pval);
      }
};

template <> class U_EXPORT UOrmTypeHandler<const char*> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(const char* val) : UOrmTypeHandler_Base((void*)val)
      {
      U_TRACE(0, "UOrmTypeHandler::UOrmTypeHandler<const char*>(%S)", val)

      U_INTERNAL_DUMP("this = %p", this)
      }

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<const char*>::bindParam(%p)", stmt)

      U_INTERNAL_DUMP("pval(%p) = %S", pval, pval)

      stmt->bindParam((const char*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<const char*>::bindResult(%p)", stmt)
      }
};

template <> class U_EXPORT UOrmTypeHandler<UStringRep> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(UStringRep& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<UStringRep>::bindParam(%p)", stmt)

      stmt->bindParam(*(UStringRep*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<UStringRep>::bindResult(%p)", stmt)

      U_ERROR("UOrmTypeHandler<UStringRep>::bindResult(): sorry, we cannot use UStringRep type to bind ORM type handler...");
      }
};

template <> class U_EXPORT UOrmTypeHandler<UString> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(UString& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<UString>::bindParam(%p)", stmt)

      stmt->bindParam(*(UString*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<UString>::bindResult(%p)", stmt)

      stmt->bindResult(*(UString*)pval);
      }
};

#ifdef U_STDCPP_ENABLE
template <> class U_EXPORT UOrmTypeHandler<istream> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(istream& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<istream>::bindParam(%p)", stmt)

      stmt->bindParam(*(istream*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<istream>::bindResult(%p)", stmt)
      }
};
#endif

template <> class U_EXPORT UOrmTypeHandler<struct tm> : public UOrmTypeHandler_Base {
public:
   explicit UOrmTypeHandler(struct tm& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<struct tm>::bindParam(%p)", stmt)

      stmt->bindParam(*(struct tm*)pval);
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<struct tm>::bindResult(%p)", stmt)
      }
};

// TEMPLATE SPECIALIZATIONS FOR CONTAINERS

template <class T> class U_EXPORT UOrmTypeHandler<UVector<T*> > : public UOrmTypeHandler_Base {
public:
   typedef UVector<T*> uvector;

   explicit UOrmTypeHandler(uvector& val) : UOrmTypeHandler_Base(&val) {}

   void bindParam(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<uvector>::bindParam(%p)", stmt)

      uvector* pvec = (uvector*)pval;

      const void** ptr = pvec->vec;
      const void** end = pvec->vec + pvec->_length;

      for (; ptr < end; ++ptr) stmt->bindParam(UOrmTypeHandler<T>(*(T*)(*ptr)));
      }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<uvector>::bindResult(%p)", stmt)

      uvector* pvec = (uvector*)pval;

      const void** ptr = pvec->vec;
      const void** end = pvec->vec + pvec->_length;

      for (; ptr < end; ++ptr) stmt->bindResult(UOrmTypeHandler<T>(*(T*)(*ptr)));
      }
};

template <> class U_EXPORT UOrmTypeHandler<UVector<UString> > : public UOrmTypeHandler<UVector<UStringRep*> > {
public:
   typedef UVector<UStringRep*> uvectorbase;

   explicit UOrmTypeHandler(UVector<UString>& val) : UOrmTypeHandler<uvectorbase>(*((uvector*)&val)) {}

   void bindParam(UOrmStatement* stmt) { ((UOrmTypeHandler<uvectorbase>*)this)->bindParam( stmt); }

   void bindResult(UOrmStatement* stmt)
      {
      U_TRACE(0, "UOrmTypeHandler<UVector<UString>>::bindResult(%p)", stmt)

      U_ERROR("UOrmTypeHandler<UVector<UString>>::bindResult(): sorry, we cannot use UVector<UString> type to bind ORM type handler...");
      }
};
#endif
