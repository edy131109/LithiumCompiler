#pragma once

#include <unordered_map>
#include <cassert>
#include <sstream>
#include <algorithm>

#include "parser.hpp"

class GeneratorWin {
public:
    inline explicit GeneratorWin(NodeProg prog)
       : m_prog(std::move(prog)) {

    }

    void gen_term(const NodeTerm* term) {
        struct TermVisitor {
            GeneratorWin& gen;
            void operator()(const NodeTermIntLit* term_int_lit) const {
                gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }
            void operator()(const NodeTermIdent* term_ident) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == term_ident->ident.value.value();
                });
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier used '" << term_ident->ident.value.value() << "'\n";
                    exit(EXIT_FAILURE);
                }
                std::stringstream offset;
                offset << "QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]";
                gen.push(offset.str());
            }

            void operator()(const NodeTermParen* term_paren) const {
                gen.gen_expr(term_paren->expr);
            }
        };
        TermVisitor visitor({.gen = *this});
        std::visit(visitor, term->var);
    }

    void gen_bin_expr(const NodeBinExpr* bin_expr) {
        struct BinExprVisitor {
            GeneratorWin& gen;
            void operator()(const NodeBinExprAdd* add) const {
                gen.gen_expr(add->rhs);
                gen.gen_expr(add->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    add rax, rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprSub* sub) const {
                gen.gen_expr(sub->rhs);
                gen.gen_expr(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    sub rax, rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprMulti* multi) const {
                gen.gen_expr(multi->rhs);
                gen.gen_expr(multi->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    mul rbx\n";
                gen.push("rax");
            }

            void operator()(const NodeBinExprDiv* div) const {
                gen.gen_expr(div->rhs);
                gen.gen_expr(div->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    xor rdx, rdx\n";
                gen.m_output << "    div rbx\n";
                gen.push("rax");
            }
        };

        BinExprVisitor visitor{.gen = *this};
        std::visit(visitor, bin_expr->var);
    }

    void gen_expr(const NodeExpr* expr) {
        struct ExprVisitor {
            GeneratorWin& gen;
            void operator()(const NodeTerm* term) const {
                gen.gen_term(term);
            }
            void operator()(const NodeBinExpr* bin_expr) const {
                gen.gen_bin_expr(bin_expr);
            }
        };

        ExprVisitor visitor{ .gen = *this };
        std::visit(visitor, expr->var);
    }

    void gen_scope(const NodeScope* scope) {
        begin_scope();
        for (const NodeStmt* stmt: scope->stmts) {
            gen_stmt(stmt);
        }
        end_scope();
    }

    void gen_stmt_set(const NodeStmtSet* stmt) {
        struct StmtSetVisitor {
            GeneratorWin& gen;
            void operator()(const NodeStmtSetExpr* stmt_set_expr) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == stmt_set_expr->ident.value.value();
                });
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier used '" << stmt_set_expr->ident.value.value() << "'\n";
                    exit(EXIT_FAILURE);
                }
                gen.gen_expr(stmt_set_expr->expr);
                gen.pop("rax");
                gen.m_output << "    add rsp, " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 + 8 << '\n';
                gen.m_output << "    push rax\n";
                gen.m_output << "    sub rsp, " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << '\n';
            }

            void operator()(const NodeStmtSetAdd* stmt_set_add) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == stmt_set_add->ident.value.value();
                });
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier used '" << stmt_set_add->ident.value.value() << "'\n";
                    exit(EXIT_FAILURE);
                }
                gen.gen_expr(stmt_set_add->expr);
                gen.pop("rax");
                gen.m_output << "    add QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "], rax\n";
            }

            void operator()(const NodeStmtSetMulti* stmt_set_add) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == stmt_set_add->ident.value.value();
                });
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier used '" << stmt_set_add->ident.value.value() << "'\n";
                    exit(EXIT_FAILURE);
                }
                gen.gen_expr(stmt_set_add->expr);
                gen.pop("rbx");
                gen.m_output << "    mov rax, QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]\n";
                gen.m_output << "    mul rbx\n";
                gen.m_output << "    mov QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "], rax\n";
            }

            void operator()(const NodeStmtSetSub* stmt_set_add) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == stmt_set_add->ident.value.value();
                });
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier used '" << stmt_set_add->ident.value.value() << "'\n";
                    exit(EXIT_FAILURE);
                }
                gen.gen_expr(stmt_set_add->expr);
                gen.pop("rax");
                gen.m_output << "    sub QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "], rax\n";
            }

            void operator()(const NodeStmtSetDiv* stmt_set_add) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == stmt_set_add->ident.value.value();
                });
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier used '" << stmt_set_add->ident.value.value() << "'\n";
                    exit(EXIT_FAILURE);
                }
                gen.gen_expr(stmt_set_add->expr);
                gen.pop("rbx");
                gen.m_output << "    mov rax, QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]\n";
                gen.m_output << "    xor rdx, rdx\n";
                gen.m_output << "    div rbx\n";
                gen.m_output << "    mov QWORD [rsp+" << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "], rax\n";
            }
        };

        StmtSetVisitor visitor{ .gen = *this };
        std::visit(visitor, stmt->var);
    }

    void gen_stmt(const NodeStmt* stmt) {
        struct StmtVisitor {
            GeneratorWin& gen;
            void operator()(const NodeStmtExit* stmt_exit) const {
                gen.gen_expr(stmt_exit->expr);
                gen.pop("rcx");
                gen.m_output << "    sub rsp, 28h\n";
                gen.m_output << "    call ExitProcess\n";
            }

            void operator()(const NodeStmtLet* stmt_let) const {
                auto it = std::find_if(gen.m_vars.cbegin(), gen.m_vars.cend(), [&](const Var& var) {
                    return var.name == stmt_let->ident.value.value();
                });
                if(it != gen.m_vars.cend()) {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.m_vars.push_back({ .name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size });
                gen.gen_expr(stmt_let->expr);
            }

            void operator()(const NodeStmtSet* stmt_set) const {
                gen.gen_stmt_set(stmt_set);
            }

            void operator()(const NodeScope* scope) const {
                gen.gen_scope(scope);
            }

            void operator()(const NodeStmtIf* stmt_if) {
                gen.gen_expr(stmt_if->expr);
                gen.pop("rax");
                std::string label = gen.create_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.gen_scope(stmt_if->scope);
                gen.m_output << label << ":\n";
            }
        };

        StmtVisitor visitor{ .gen = *this };
        std::visit(visitor, stmt->var);
    }

    [[nodiscard]] std::string gen_prog() {
        m_output << "extern ExitProcess\n\nglobal _start\nsection .text\n_start:\n";

        for (const NodeStmt* stmt : m_prog.stmts) {
            gen_stmt(stmt);
        }

        m_output << "    sub rsp, 28h\n";
        m_output << "    mov ecx, 0\n";
        m_output << "    call ExitProcess\n";
        return m_output.str();
    }
private:

    void push(const std::string& reg) {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    void pop(const std::string& reg) {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    void begin_scope() {
        m_scopes.push_back(m_vars.size());
    }

    void end_scope() {
        size_t pop_count = m_vars.size() - m_scopes.back();
        m_output << "    add rsp, " << pop_count * 8 << '\n';
        m_stack_size -= pop_count;
        for (size_t i = 0; i < pop_count; i++) {
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }

    std::string create_label() {
        return "label" + std::to_string(m_label_count++);
    }

    struct Var {
        std::string name;
        size_t stack_loc;
    };

    const NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::vector<Var> m_vars {};
    std::vector<size_t> m_scopes {};
    size_t m_label_count = 0;
};