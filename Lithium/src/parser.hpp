#pragma once

#include <variant>
#include <cassert>

#include "tokenization.hpp"
#include "arena.hpp"

struct NodeTermIntLit {
    Token int_lit;
};

struct NodeTermIdent {
    Token ident;
};

struct NodeExpr;

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprMulti {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprSub*, NodeBinExprMulti*, NodeBinExprDiv*> var;
};

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtSetExpr {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtSetAdd {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtSetMulti {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtSetSub {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtSetDiv {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtSet {
    std::variant<NodeStmtSetExpr*, NodeStmtSetAdd*, NodeStmtSetMulti*, NodeStmtSetSub*, NodeStmtSetDiv*> var;
};

struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt*> stmts;
};

struct NodeIfPred;

struct NodeIfPredElseIf {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeIfPredElse {
    NodeScope* scope;
};

struct NodeIfPred {
    std::variant<NodeIfPredElseIf*, NodeIfPredElse*> var;
};

struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
    std::optional<NodeIfPred*> pred;
};

struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeStmtSet*, NodeScope*, NodeStmtIf*> var;
};

struct NodeProg {
    std::vector<NodeStmt*> stmts;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens, std::string srcName) :
        m_tokens(std::move(tokens)),
        m_allocator(1024 * 1024 * 4), // 4 mb
        m_srcName(srcName)
        {
            
        }

        void error(const std::string& msg, int line = -1, int col = -1) {
            if (line == -1) {
                line = peek(-1).value().line;
            }
            if (col == -1) {
                col = peek(-1).value().col + 1;
            }
            std::cerr << m_srcName << ":" << line << ":" << col << ": parse_error: " << msg << std::endl;
        }

        void error_expected(const std::string& msg, const int line, const int col = -1) {
            error("Expected " + msg, line, col);
        }

        std::optional<NodeTerm*> parse_term() {
            if (auto int_lit = try_consume(TokenType::int_lit)) {
                auto term_int_lit = m_allocator.emplace<NodeTermIntLit>(int_lit.value());
                auto term = m_allocator.emplace<NodeTerm>(term_int_lit);
                return term;
            }
            if (auto ident = try_consume(TokenType::ident)) {
                auto expr_ident = m_allocator.emplace<NodeTermIdent>(ident.value());
                auto term = m_allocator.emplace<NodeTerm>(expr_ident);
                return term;
            }
            if (const auto open_paren = try_consume(TokenType::open_paren)) {
                auto expr = parse_expr();
                if (!expr.has_value()) {
                    error_expected("expr", open_paren.value().line);
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paren, "Expected ')'", open_paren.value().line);
                auto term_paren = m_allocator.emplace<NodeTermParen>(expr.value());
                auto term = m_allocator.emplace<NodeTerm>(term_paren);
                return term;
            }
            return {};
        }

        std::optional<NodeExpr*> parse_expr(const int min_prec = 0) {
            std::optional<NodeTerm*> term_lhs = parse_term();
            if (!term_lhs.has_value()) {
                return {};
            }
            auto expr_lhs = m_allocator.emplace<NodeExpr>(term_lhs.value());

            while (true) {
                std::optional<Token> curr_tok = peek();
                std::optional<int> prec;
                if (curr_tok.has_value()) {
                    prec = bin_prec(curr_tok->type);
                    if (!prec.has_value() || prec < min_prec) {
                        break;
                    }      
                }
                else {
                    break;
                }
                const Token op = consume();
                const int next_min_prec = prec.value() + 1;
                auto expr_rhs = parse_expr(next_min_prec);
                if (!expr_rhs.has_value()) {
                    error("Unable to parse expression", op.line);
                    exit(EXIT_FAILURE);
                }
                auto expr = m_allocator.emplace<NodeBinExpr>();
                auto expr_lhs2 = m_allocator.emplace<NodeExpr>();
                if (op.type == TokenType::plus) {
                    expr_lhs2->var = expr_lhs->var;
                    auto add = m_allocator.emplace<NodeBinExprAdd>(expr_lhs2, expr_rhs.value());
                    expr->var = add;
                }
                else if (op.type == TokenType::minus) {
                    expr_lhs2->var = expr_lhs->var;
                    auto sub = m_allocator.emplace<NodeBinExprSub>(expr_lhs2, expr_rhs.value());
                    expr->var = sub;
                }
                else if (op.type == TokenType::star) {
                    expr_lhs2->var = expr_lhs->var;
                    auto multi = m_allocator.emplace<NodeBinExprMulti>(expr_lhs2, expr_rhs.value());
                    expr->var = multi;
                }
                else if (op.type == TokenType::fslash) {
                    expr_lhs2->var = expr_lhs->var;
                    auto div = m_allocator.emplace<NodeBinExprDiv>(expr_lhs2, expr_rhs.value());
                    expr->var = div;
                }
                else {
                    assert(false); // Unreachable
                }
                expr_lhs->var = expr;
            }
            return expr_lhs;
        }

        std::optional<NodeScope*> parse_scope() {
            if (!try_consume(TokenType::open_curly).has_value()) {
                return {};
            }
            const auto scope = m_allocator.emplace<NodeScope>();
            while (auto stmt = parse_stmt()) {
                scope->stmts.push_back(stmt.value());
            }
            try_consume(TokenType::close_curly, "Expected '}'");
            return scope;
        }

        std::optional<NodeStmtSet*> parse_stmt_set() {
            Token ident = consume();
            auto stmt_set = m_allocator.emplace<NodeStmtSet>();
            if (peek().value().type == TokenType::eq) {
                auto stmt_set_expr = m_allocator.emplace<NodeStmtSetExpr>();
                stmt_set_expr->ident = ident;
                consume();
                if (const auto expr = parse_expr()) {
                    stmt_set_expr->expr = expr.value();
                }
                else {
                    error("Invalid expression", ident.line);
                    exit(EXIT_FAILURE);
                }
                stmt_set->var = stmt_set_expr;
            }
            else if (peek().value().type == TokenType::pluseq) {
                auto stmt_set_expr = m_allocator.emplace<NodeStmtSetAdd>();
                stmt_set_expr->ident = ident;
                consume();
                if (const auto expr = parse_expr()) {
                    stmt_set_expr->expr = expr.value();
                }
                else {
                    error("Invalid expression", ident.line);
                    exit(EXIT_FAILURE);
                }
                stmt_set->var = stmt_set_expr;
            }
            else if (peek().value().type == TokenType::stareq) {
                auto stmt_set_expr = m_allocator.emplace<NodeStmtSetMulti>();
                stmt_set_expr->ident = ident;
                consume();
                if (const auto expr = parse_expr()) {
                    stmt_set_expr->expr = expr.value();
                }
                else {
                    error("Invalid expression", ident.line);
                    exit(EXIT_FAILURE);
                }
                stmt_set->var = stmt_set_expr;
            }
            else if (peek().value().type == TokenType::minuseq) {
                auto stmt_set_expr = m_allocator.emplace<NodeStmtSetSub>();
                stmt_set_expr->ident = ident;
                consume();
                if (const auto expr = parse_expr()) {
                    stmt_set_expr->expr = expr.value();
                }
                else {
                    error("Invalid expression", ident.line);
                    exit(EXIT_FAILURE);
                }
                stmt_set->var = stmt_set_expr;
            }
            else if (peek().value().type == TokenType::fslasheq) {
                auto stmt_set_expr = m_allocator.emplace<NodeStmtSetDiv>();
                stmt_set_expr->ident = ident;
                consume();
                if (const auto expr = parse_expr()) {
                    stmt_set_expr->expr = expr.value();
                }
                else {
                    error("Invalid expression", ident.line);
                    exit(EXIT_FAILURE);
                }
                stmt_set->var = stmt_set_expr;
            }
            else {
                return {};
            }
            return stmt_set;
        }

        std::optional<NodeIfPred*> parse_if_pred() {
            if (try_consume(TokenType::else_)) {
                if (try_consume(TokenType::if_)) {
                    // else if
                    try_consume(TokenType::open_paren, "Expected '('");
                    const auto elseif = m_allocator.alloc<NodeIfPredElseIf>();
                    if (const auto expr = parse_expr()) {
                        elseif->expr = expr.value();
                    }
                    else {
                        error("Expected expression");
                        exit(EXIT_FAILURE);
                    }
                    try_consume(TokenType::close_paren, "Expected ')'");
                    if (const auto scope = parse_scope()) {
                        elseif->scope = scope.value();
                    }
                    else {
                        error("Invalid scope");
                        exit(EXIT_FAILURE);
                    }
                    elseif->pred = parse_if_pred();
                    auto if_pred = m_allocator.emplace<NodeIfPred>(elseif);
                    return if_pred;
                }
                else {
                    // else
                    const auto else_ = m_allocator.alloc<NodeIfPredElse>();
                    if (const auto scope = parse_scope()) {
                        else_->scope = scope.value();
                    }
                    else {
                        error("Invalid scope");
                        exit(EXIT_FAILURE);
                    }
                    auto if_pred = m_allocator.emplace<NodeIfPred>(else_);
                    return if_pred;
                }
            }
            // no pred found
            return {};
        }

        std::optional<NodeStmt*> parse_stmt() {
            if (peek().value().type == TokenType::_exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
                consume();
                consume();
                auto stmt_exit = m_allocator.emplace<NodeStmtExit>();
                if (const auto node_expr = parse_expr()) {
                    stmt_exit->expr = node_expr.value();
                } else {
                    error("Invalid expression");
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paren, "Expected ')'");
                try_consume(TokenType::semi, "Expected ';'");
                auto stmt = m_allocator.emplace<NodeStmt>();
                stmt->var = stmt_exit;
                return stmt;
            }
            if (peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value() && peek(1).value().type == TokenType::ident && peek(2).has_value() && peek(2).value().type == TokenType::eq) {
                consume();
                auto stmt_let = m_allocator.emplace<NodeStmtLet>();
                stmt_let->ident = consume();
                consume();
                if (auto expr = parse_expr()) {
                    stmt_let->expr = expr.value();
                }
                else {
                    error("Invalid expression", stmt_let->ident.line);
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::semi, "Expected ';'", stmt_let->ident.line);
                auto stmt = m_allocator.emplace<NodeStmt>();
                stmt->var = stmt_let;
                return stmt;
            }
            if (peek().has_value() && peek().value().type == TokenType::ident && peek(1).has_value()) {
                auto stmt = m_allocator.emplace<NodeStmt>();
                if (auto stmt_set = parse_stmt_set()) {
                    stmt->var = stmt_set.value();
                }
                else {
                    error("Invalid set statement");
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::semi, "Expected ';'");
                return stmt;
            }
            if (peek().has_value() && peek().value().type == TokenType::open_curly) {
                if (auto scope = parse_scope()) {
                    auto stmt = m_allocator.emplace<NodeStmt>();
                    stmt->var = scope.value();
                    return stmt;
                }
                error("Invalid scope");
                exit(EXIT_FAILURE);
            }
            if (auto if_ = try_consume(TokenType::if_)) {
                try_consume(TokenType::open_paren, "Expected '('", if_.value().line);
                auto stmt_if = m_allocator.emplace<NodeStmtIf>();
                if (auto expr = parse_expr()) {
                    stmt_if->expr = expr.value();
                }
                else {
                    error("Invalid expression", if_.value().line);
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::close_paren, "Expected ')'", if_.value().line);
                if (auto scope = parse_scope()) {
                    stmt_if->scope = scope.value();
                }
                else {
                    error("Invalid scope");
                    exit(EXIT_FAILURE);
                }
                stmt_if->pred = parse_if_pred();
                auto stmt = m_allocator.emplace<NodeStmt>(stmt_if);
                return stmt;
            }
            return {};
        }

        std::optional<NodeProg> parse_prog() {
            NodeProg prog;
            while (peek().has_value()) {
                if (auto stmt = parse_stmt()) {
                    prog.stmts.push_back(stmt.value());
                }
                else {
                    error("Invalid statement");
                    exit(EXIT_FAILURE);
                }
            }
            return prog;
        }

private:
    [[nodiscard]] std::optional<Token> peek(const int offset = 0) const {
        if (m_index + offset >= m_tokens.size()) {
            return {};
        }
        return m_tokens.at(m_index + offset);
    }

    Token consume() {
        return m_tokens.at(m_index++);
    }

    Token try_consume(const TokenType type, const std::string& err_msg, int line = -1, int col = -1) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        error(err_msg, line, col);
        exit(EXIT_FAILURE);
    }

    std::optional<Token> try_consume(const TokenType type) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        return {};
    }

    const std::string m_srcName;
    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};