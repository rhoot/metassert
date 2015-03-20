// Copyright 2015 Johan Sköld. All rights reserved.
// License: http://www.opensource.org/licenses/BSD-2-Clause

#ifndef METASSERT_H
#define METASSERT_H

#include <sstream>      // <- For building the assertion failure string
#include <iostream>     // <- For outputting assertion messages

/*

    METASSERT(expr)

    Perform an assertion. An expression tree is constructed at compile time, so
    we can inspect the expression at runtime in case of failed assertions. As
    it is constructed at compile time however, only the expression itself will
    be executed at runtime if the assertion is successful.

    Note:
    - Does not actually *do* anything on assertion failure, other than
      printing to stdout.
    - All operators are not supported (though most could be added with some
      modification).
    - If multiple operators are part of the expression, it's up to the compiler
      how many we can actually catch.

    Example:
        int a = rand();
        int b = rand();
        METASSERT(a == b);

    Outputs:
        Failed: "a == b" (41 == 18467), in file main.cpp line 6

    Generated asm (VC12, x64):
        ; int a = rand();
            call    QWORD PTR __imp_rand
            mov     DWORD PTR a$[rsp], eax

        ; int b = rand();
            call    QWORD PTR __imp_rand
            mov     DWORD PTR b$[rsp], eax

        ; DEFINE_OPERATOR(==, Equal);
            cmp     DWORD PTR a$[rsp], eax

        ; METASSERT(a == b);
            je      $LN102@main

        ; <failure code>
            ...

*/

namespace metassert {
    
/// Expression taking two components.
/// - Lhs: Type of the left hand side.
/// - Rhs: Type of the right hand side.
/// - Op:  Type of a functor to perform the operation.
template <typename Lhs, typename Rhs, typename Op>
class Expression {
    using ResultType = typename std::result_of<Op(const Lhs&, const Rhs&)>::type;

public:
    /// Construct the Expression.
    Expression(const Lhs& lhs, const Rhs& rhs, Op&& op)
        : m_lhs(lhs)
        , m_rhs(rhs)
        , m_op(std::forward<Op>(op))
    { }

    /// Return the result of the operation.
    operator typename ResultType() const {
        return m_op(m_lhs, m_rhs);
    }
    
    /// Write the expression to an output stream.
    /// - stream: Stream to write to.
    /// - expr:   Expression to write.
    friend std::ostream& operator<<(std::ostream& stream, const Expression& expr) {
        stream << expr.m_lhs << " " << expr.m_op << " " << expr.m_rhs;
        return stream;
    }

private:
    const Lhs& m_lhs;
    const Rhs& m_rhs;
    Op m_op;
};

/// Expression builder
template <typename Lhs>
class ExpressionBuilder {
public:
    ExpressionBuilder(const Lhs& lhs)
        : m_lhs(lhs)
    { }

    const Lhs& GetExpression() { return m_lhs; }
    
private:
    const Lhs& m_lhs;
};

/// Define a new operator for the Expression builder
#define DEFINE_OPERATOR(op, name)                                                                                                               \
    template <typename T1, typename T2> struct name { auto operator()(const T1& a, const T2& b) const -> decltype(a op b) { return a op b; } }; \
    template <typename T1, typename T2> std::ostream& operator<<(std::ostream& os, const name<T1, T2>& o) { os << #op; return os; }             \
    template <typename Lhs, typename Rhs>                                                                                                       \
    ExpressionBuilder<Expression<Lhs, Rhs, name<Lhs, Rhs>>> operator##op (ExpressionBuilder<Lhs>& builder, const Rhs& rhs) {                    \
        Expression<Lhs, Rhs, name<Lhs, Rhs>> expr(builder.GetExpression(), rhs, name<Lhs, Rhs>());                                              \
        return ExpressionBuilder<Expression<Lhs, Rhs, name<Lhs, Rhs>>>(std::move(expr));                                                        \
    }

// Expression builder operators
DEFINE_OPERATOR(+,  Add);
DEFINE_OPERATOR(-,  Subtract);
DEFINE_OPERATOR(*,  Multiply);
DEFINE_OPERATOR(/,  Divide);
DEFINE_OPERATOR(==, Equal);
DEFINE_OPERATOR(!=, NotEqual);
DEFINE_OPERATOR(<,  LessThan);
DEFINE_OPERATOR(<=, LessOrEqual);
DEFINE_OPERATOR(>,  GreaterThan);
DEFINE_OPERATOR(>=, GreaterOrEqual);

#undef DEFINE_OPERATOR

/// Helper struct to begin building an expression
struct Build { };

template <typename Lhs>
ExpressionBuilder<Lhs> operator->* (Build&, const Lhs& lhs) {
    return ExpressionBuilder<Lhs>(lhs);
}

/// Print an assertion failure message
inline void AssertFail(const std::string& msg, const char file[], unsigned line) {
    std::cout << "Failed: " << msg << ", in file " << file << " line " << line << std::endl;
}

} // namespace metassert

/// Assert that the given expression is true. If it is not, the original
/// expression - as well as the evaluated expression - are output to stdout.
/// - expr: Expression to test for truthness.
#define METASSERT(expr)                                                 \
    do {                                                                \
        auto _expr_ = (metassert::Build()->*expr).GetExpression();      \
        if (!_expr_) {                                                  \
            std::stringstream _stream_;                                 \
            _stream_ << "\"" #expr "\"" << " (" << _expr_ << ")";       \
            metassert::AssertFail(_stream_.str(), __FILE__, __LINE__);  \
        }                                                               \
    } while (0,0)

#endif // METASSERT_H