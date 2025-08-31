#pragma once
#include "./tokenization.hpp"
#include <variant>
struct NodeTermIntLit {
    Token int_lit;
};
struct NodeTermIdentifier {
    Token ident;
};
struct NodeBinExpr;

struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdentifier*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};
struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

// struct NodeBinExprMul {
//     NodeExpr* lhs;
//     NodeExpr* rhs;
// };

struct NodeBinExpr {
    NodeBinExprAdd* var;
};


struct NodeStmtExit {
    NodeExpr* expression;
};
struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};
struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*> var;
};
struct NodeProg {
    std::vector<NodeStmt*> stmts;
};



class Parser {
public:
    inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)),  m_allocator(1024*1024*4) {

    }

    inline std::optional<NodeTerm*> parse_term() {
        if (auto int_lit = try_consume(TokenType::int_lit)) {
            auto node_term_int_lit = m_allocator.alloc<NodeTermIntLit>();
            node_term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var=node_term_int_lit;
            return term;
        } else if (auto ident = try_consume(TokenType::ident)) {
            auto term_ident = m_allocator.alloc<NodeTermIdentifier>();
            term_ident->ident = ident.value();
            auto term = m_allocator.alloc<NodeTerm>();
            term->var = term_ident;
            return term;
        }
        else {
            return {};
        }
    }

    inline std::optional<NodeExpr*> parse_expr() {
        if (auto term = parse_term()) {
            if (try_consume(TokenType::plus).has_value()) {
                auto bin_expr = m_allocator.alloc<NodeBinExpr>();
                auto bin_exp_add = m_allocator.alloc<NodeBinExprAdd>();
                auto lhs_expr = m_allocator.alloc<NodeExpr>();
                lhs_expr->var = term.value();
                bin_exp_add->lhs = lhs_expr;
                if (auto rhs = parse_expr()) {
                    bin_exp_add->rhs = rhs.value();
                    bin_expr->var = bin_exp_add;
                    auto expr = m_allocator.alloc<NodeExpr>();
                    expr->var = bin_expr;
                    return expr;
                } else {
                        std::cerr << "Expected Expression." << std::endl;
                        exit(EXIT_FAILURE);
                    }
            } else {
                auto expr = m_allocator.alloc<NodeExpr>();
                expr->var = term.value();
                return expr;
            }
        } else {
            return {};
        }

    }
    std::optional<NodeStmt*> parse_stmt() {
        if (peak().has_value() && peak().value().type == TokenType::exit &&
            peak(1).has_value() && peak(1).value().type == TokenType::open_paren) {
            consume();
            consume();
            NodeStmtExit* stmt_exit;
            if (auto node_expr = parse_expr()) {
                stmt_exit = m_allocator.alloc<NodeStmtExit>();
                stmt_exit->expression = node_expr.value();
            } else {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected ')'");
            try_consume(TokenType::semi, "Expected ';'");
            auto node_stmt = m_allocator.alloc<NodeStmt>();
            node_stmt->var = stmt_exit;
            return node_stmt;
        }
        else if (try_consume(TokenType::let).has_value()) {
            if (auto ident = try_consume(TokenType::ident)) {
                auto stmt = m_allocator.alloc<NodeStmtLet>();
                stmt->ident=ident.value(); //ident
                try_consume(TokenType::eq, "Expected '='");
                if (auto expr = parse_expr()) { //expr
                    stmt->expr = expr.value();
                    try_consume(TokenType::semi, "Expected ';'");
                    auto node_stmt = m_allocator.alloc<NodeStmt>();
                    node_stmt->var = stmt;
                    return node_stmt;
                } else {
                    std::cerr << "Invalid expression." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }
        return {};
    }



    inline std::optional<NodeProg> parse_prog() {
        NodeProg prog;
        while (peak().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            } else {
                std::cerr << "Invalid statement." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return prog;
    }


private:
    [[nodiscard]] inline std::optional<Token> peak(int offset = 0) const {
        if (m_index + offset >= m_tokens.size()) {
            return {};
        }
        else {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume() {
        return m_tokens.at(m_index++);
    }

    inline Token try_consume(TokenType type, const std::string& err_msg) {
        if (peak().has_value() && peak().value().type == type) {
            return consume();
        } else {
            std::cerr<<err_msg<<std::endl;
            exit(EXIT_FAILURE);
        }
    }
    inline std::optional<Token> try_consume(TokenType type) {
        if (peak().has_value() && peak().value().type == type) {
            return consume();
        } else {
            return {};
        }
    }
    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;
};
