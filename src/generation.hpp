#pragma once
#include <algorithm>
#include <sstream>
#include "./parser.hpp"

class Generator {
public:

    inline explicit Generator(NodeProg root) : m_root(std::move(root)) {};

    void gen_term(const NodeTerm* term) {
        struct TermVisitor {
            Generator& gen;
            void operator()(const NodeTermIntLit* term_int_lit) const {
                gen.m_output_text << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }
            void operator()(const NodeTermIdentifier* term_identifier ) const {
                auto it = std::find_if(gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var& var){return var.name == term_identifier->ident.value.value();});
                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared identifier: " << term_identifier->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                    }
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen.m_stack_size-(*it).stack_loc-1) * 8 << "]";
                gen.push(offset.str());
            }
            void operator()(const NodeTermParen* term_paren ) const {
                gen.gen_expr(term_paren->expr);
            }
        };
        TermVisitor visitor({.gen = *this});
        std::visit(visitor, term->var);
    }
    void gen_bin_exp(const NodeBinExpr* bin_expr) {
        struct BinExprVisitor {
            Generator& gen;
            void operator()(const NodeBinExprAdd* add_expr) {
                gen.gen_expr(add_expr->lhs);
                gen.gen_expr(add_expr->rhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output_text << "    ADD rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprMul* mul_expr) {
                gen.gen_expr(mul_expr->lhs);
                gen.gen_expr(mul_expr->rhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output_text << "    mul rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprSub* sub_expr) {
                gen.gen_expr(sub_expr->rhs); //because this is a stack we need first the rhs then lhs
                gen.gen_expr(sub_expr->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output_text << "    sub rax, rbx\n";
                gen.push("rax");
            }
            void operator()(const NodeBinExprDiv* sub_expr) {
                gen.gen_expr(sub_expr->rhs); //because this is a stack we need first the rhs then lhs
                gen.gen_expr(sub_expr->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output_text << "    div rbx\n";
                gen.push("rax");
            }
        };

        BinExprVisitor visitor{.gen = *this};
        std::visit(visitor, bin_expr->var);
    }
    void gen_expr(const NodeExpr* expr) {
        struct ExprVisitor {
            Generator& gen;
            void operator()(const NodeTerm* term) {
                gen.gen_term(term);
            }
            void operator()(const NodeBinExpr* bin_expr) {
                gen.gen_bin_exp(bin_expr);
            }
        };

        ExprVisitor visitor{.gen = *this};
        std::visit(visitor, expr->var);
    }
    void gen_scope(const NodeScope* node_scope) {
        begin_scope();
        for (const NodeStmt* stmt : node_scope->stmts) {
            gen_stmt(stmt);
        }
        end_scope();
    }
    void gen_stmt(const NodeStmt* stmt) {
           struct StmtVisitor {
               Generator& gen;
               void operator()(const NodeStmtExit* stmt_exit) {
                   gen.gen_expr(stmt_exit->expression);
                   gen.m_output_text << "    mov rax, 60\n";
                   gen.pop("rdi");
                   gen.m_output_text << "    syscall\n";
               }
               void operator()(const NodeStmtLet* stmt_let) {
                   auto it = std::find_if(gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var& var){return var.name == stmt_let->ident.value.value();});
                   if (it != gen.m_vars.cend()) {
                       std::cerr << "Identifier already used." << std::endl;
                       exit(EXIT_FAILURE);
                   }
                   gen.m_vars.push_back({ .name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size });
                   gen.gen_expr(stmt_let->expr);
               }
               void operator()(const NodeStmtPrint* stmt_print) {
                   if (gen.m_resb_size_print == 0) {
                       gen.m_resb_size_print=1;
                       gen.m_output_bss << "    char resb 1\n"; //TODO when adding printing strings, add resb_size_print
                   }
                   for (const NodeExpr* expr : stmt_print->expr) {
                       gen.gen_expr(expr);
                       gen.pop("rax");
                       gen.m_output_text << "    mov [char], al\n";
                       gen.m_output_text << "    mov rsi, char\n";
                       gen.m_output_text << "    mov rdi, 1\n";
                       gen.m_output_text << "    mov rdx, 1\n";
                       gen.m_output_text << "    mov rax, 1\n    syscall\n";
                   }
               }
               void operator()(const NodeStmtAssign* stmt_assign) {
                   auto it = std::find_if(gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var& var){return var.name == stmt_assign->ident.value.value();});
                    if (it == gen.m_vars.cend()) {
                        std::cerr << "Identifier not found." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                   auto var = (*it);
                   gen.gen_expr(stmt_assign->expr);
                   gen.pop("rax");
                   gen.m_output_text << "    mov QWORD [rsp + " << (gen.m_stack_size-var.stack_loc-1) * 8 << "], rax\n";
               }
               void operator()(const NodeScope* stmt_scope) {
                  gen.gen_scope(stmt_scope);
               }
               void operator()(const NodeStmtIf* stmt_if) {
                    gen.gen_expr(stmt_if->expr);
                   gen.pop("rax");
                   auto lbl = gen.create_label();
                   gen.m_output_text << "    test rax, rax\n";
                   gen.m_output_text << "    jz " << lbl << "\n";
                   gen.gen_scope(stmt_if->scope);
                   gen.m_output_text << lbl << ":\n";
               }
           };

        StmtVisitor visitor {.gen = *this};
        std::visit(visitor, stmt->var);
    }

    std::string gen_prog(){
        m_output_bss << "section .bss\n";
        m_output_text << "section .text\nglobal _start\n_start:\n";
        for (const NodeStmt* stmt : m_root.stmts) {
            gen_stmt(stmt);
        }
        m_output_text << "    mov rax, 60\n";
        m_output_text << "    mov rdi, 0\n";
        m_output_text << "    syscall";

        (m_output_bss << m_output_text.str());
        return m_output_bss.str();
    }
private:
    void push(const std::string& reg) {
        m_output_text << "    push " << reg << "\n";
        m_stack_size++;
    }
    void pop(const std::string& reg) {
        m_output_text << "    pop " << reg << "\n";
        m_stack_size--;
    }
    void begin_scope() {
        m_scopes.push_back(m_vars.size());
    }
    void end_scope() {
        size_t pop_count = m_vars.size()-m_scopes.back();
        m_output_text << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_size -= pop_count;
        for (int i = 0; i<pop_count; i++) {
            m_vars.pop_back();
        }
        m_scopes.pop_back();
    }
    std::string create_label() {
        std::stringstream ss;
        ss<<"label" << m_label_count++;
        return ss.str();
    }
    struct Var {
        std::string name;
        size_t stack_loc;
    };

    NodeProg m_root;
    std::stringstream m_output_text;
    std::stringstream m_output_bss;
    size_t m_stack_size = 0;
    size_t m_resb_size_print = 0;
    std::vector<Var> m_vars {};
    std::vector<size_t> m_scopes {};
    int m_label_count = 0;
};
