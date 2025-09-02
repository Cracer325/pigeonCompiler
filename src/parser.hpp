#pragma once
#include "./tokenization.hpp"
#include <variant>
#include <string>
struct NodeTermIntLit {
    Token int_lit;
};
struct NodeTermIdentifier {
    Token ident;
};
struct NodeBinExpr;
struct NodeExpr;
struct NodeTermParen {
    NodeExpr* expr;
};
struct NodeTerm {
    std::variant<NodeTermIntLit*, NodeTermIdentifier*, NodeTermParen*> var;
};

struct NodeExpr {
    std::variant<NodeTerm*, NodeBinExpr*> var;
};
struct NodeBinExprAdd {
    NodeExpr* lhs;
    NodeExpr* rhs;
};
struct NodeBinExprSub {
    NodeExpr* lhs;
    NodeExpr* rhs;
};
struct NodeBinExprDiv {
    NodeExpr* lhs;
    NodeExpr* rhs;
};
struct NodeBinExprMul {
    NodeExpr* lhs;
    NodeExpr* rhs;
};

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*, NodeBinExprMul*, NodeBinExprSub*, NodeBinExprDiv*> var;
};

struct NodeStmtExit {
    NodeExpr* expression;
};
struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};
struct NodeStmtPrint {
    std::vector<NodeExpr*>  expr;
};
struct NodeStmt;

struct NodeScope {
    std::vector<NodeStmt*>  stmts;
};
struct NodeStmtIf {
    NodeExpr* expr;
    NodeScope* scope;
};
struct NodeStmtAssign {
    Token ident;
    NodeExpr* expr;
};
struct NodeStmt {
    std::variant<NodeStmtExit*, NodeStmtLet*, NodeStmtPrint*, NodeStmtAssign*, NodeScope*, NodeStmtIf*> var;
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
        else if (auto open_paren = try_consume(TokenType::open_paren)) {
            if (auto expr = parse_expr()) {
                if (auto close_paren = try_consume(TokenType::close_paren)) {
                    auto term_paren = m_allocator.alloc<NodeTermParen>();
                    term_paren->expr = expr.value();
                    auto term = m_allocator.alloc<NodeTerm>();
                    term->var = term_paren;
                    return term;
                } else {
                    std::cerr << "Expected ')'." << std::endl;
                    exit(EXIT_FAILURE);
                }
            } else {
                std::cerr << "Expected expression." << std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (auto minus = try_consume(TokenType::minus)) {
            if (auto int_lit = try_consume(TokenType::int_lit)) {
                auto node_term_int_lit = m_allocator.alloc<NodeTermIntLit>();
                int_lit.value().value = "-" + int_lit.value().value.value();
                node_term_int_lit->int_lit = int_lit.value();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var=node_term_int_lit;
                return term;
            }
        } else if (auto apo = try_consume(TokenType::apo)) {
            if (auto lit = try_consume(TokenType::ident)) {
                if (!lit.value().value.has_value() || lit.value().value.value().length() != 1) {
                    std::cerr << "Expected a char." << std::endl;
                    exit(EXIT_FAILURE);
                }
                try_consume(TokenType::apo, "Expected '.");
                auto node_char_lit = m_allocator.alloc<NodeTermIntLit>();
                int val = static_cast<int>(lit.value().value.value().at(0));
                node_char_lit->int_lit.type = TokenType::int_lit;
                std::stringstream str;
                str << val;
                node_char_lit->int_lit.value = str.str();
                auto term = m_allocator.alloc<NodeTerm>();
                term->var=node_char_lit;
                return term;
            }
            if (auto bslash = try_consume(TokenType::bslash)) {
                if (auto lit = try_consume(TokenType::ident)) {
                    if (!lit.value().value.has_value() || lit.value().value.value().length() != 1) {
                        std::cerr << "Expected a char." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    try_consume(TokenType::apo, "Expected '.");
                    auto node_char_lit = m_allocator.alloc<NodeTermIntLit>();
                    if (lit.value().value.value() == "n") {
                        node_char_lit->int_lit.type = TokenType::int_lit;
                        node_char_lit->int_lit.value = "10";
                        auto term = m_allocator.alloc<NodeTerm>();
                        term->var=node_char_lit;
                        return term;
                    }
                    else {
                        std::cerr << "Unsupported \\." << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        else {
            return {};
        }
    }

    inline std::optional<NodeExpr*> parse_expr(int min_prec = 0) {
        if (auto term_lhs = parse_term()) {
            auto expr_lhs = m_allocator.alloc<NodeExpr>();
            expr_lhs->var = term_lhs.value();
            while (true) {
                auto cur_tok = peak();
                if ((!cur_tok.has_value()) || !is_bin_op(cur_tok->type) || get_prec(cur_tok->type).value() < min_prec) {
                    break;
                }
                int prec = get_prec(cur_tok->type).value();
                int next_min_prec = prec+1;
                auto op = consume();
                auto expr_rhs = parse_expr(next_min_prec);
                if (!expr_rhs.has_value()) {
                    std::cerr << "Unable to parse expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto expr = m_allocator.alloc<NodeBinExpr>();
                if (op.type == TokenType::plus) {
                    auto add = m_allocator.alloc<NodeBinExprAdd>();
                    add->lhs = m_allocator.alloc<NodeExpr>();
                    add->lhs->var = expr_lhs->var;
                    add->rhs = expr_rhs.value();
                    expr->var = add;
                } else if (op.type == TokenType::star) {
                    auto multi = m_allocator.alloc<NodeBinExprMul>();
                    multi->lhs = m_allocator.alloc<NodeExpr>();
                    multi->lhs->var = expr_lhs->var;
                    multi->rhs = expr_rhs.value();
                    expr->var = multi;
                }
                else if (op.type == TokenType::minus) {
                    auto sub = m_allocator.alloc<NodeBinExprSub>();
                    sub->lhs = m_allocator.alloc<NodeExpr>();
                    sub->lhs->var = expr_lhs->var;
                    sub->rhs = expr_rhs.value();
                    expr->var = sub;
                }
                else if (op.type == TokenType::slash) {
                    auto div = m_allocator.alloc<NodeBinExprDiv>();
                    div->lhs = m_allocator.alloc<NodeExpr>();
                    div->lhs->var = expr_lhs->var;
                    div->rhs = expr_rhs.value();
                    expr->var = div;
                }
                expr_lhs->var = expr;
            }
            return expr_lhs;
        }
        return {};
    }
    std::optional<NodeScope*> parse_scope() {
        if (!try_consume(TokenType::open_curly).has_value()) {
            return {};
        }
        auto scope = m_allocator.alloc<NodeScope>();
        while (auto stmt = parse_stmt()) {
            scope->stmts.push_back(stmt.value());
        }
        try_consume(TokenType::close_curly, "Expected '}'");
        return scope;
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
        } else if (try_consume(TokenType::print).has_value()) {
            auto stmt_print = m_allocator.alloc<NodeStmtPrint>();
            if (!try_consume(TokenType::open_paren).has_value()) {
                std::cerr << "Expected '('"<<std::endl;
                exit(EXIT_FAILURE);
            }
            if (auto print_expr = parse_expr()) {
                stmt_print->expr.push_back(print_expr.value());
                while (try_consume(TokenType::comma).has_value()) {
                    if ((print_expr = parse_expr()).has_value()) {
                        stmt_print->expr.push_back(print_expr.value());
                    } else {
                        std::cerr << "Expected expression"<<std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                if (!try_consume(TokenType::close_paren).has_value()) {
                    std::cerr << "Expected ')'"<<std::endl;
                    exit(EXIT_FAILURE);
                }
                if (!try_consume(TokenType::semi).has_value()) {
                    std::cerr << "Expected ';'"<<std::endl;
                    exit(EXIT_FAILURE);
                }
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmt_print;
                return stmt;
            } else {
                std::cerr << "Expected expression"<<std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (auto ident = try_consume(TokenType::ident)) {
            try_consume(TokenType::eq, "Expected '='.");
            auto stmt_assign = m_allocator.alloc<NodeStmtAssign>();
            stmt_assign->ident = ident.value();
            if (auto expr = parse_expr()) {
                stmt_assign->expr = expr.value();
                auto stmt = m_allocator.alloc<NodeStmt>();
                stmt->var = stmt_assign;
                try_consume(TokenType::semi, "Expected ';'.");
                return stmt;
            } else {
                std::cerr<<"Expected expression."<<std::endl;
                exit(EXIT_FAILURE);
            }
        } else if (auto scope = parse_scope()) {
            auto stmt = m_allocator.alloc<NodeStmt>();
            stmt->var = scope.value();
            return stmt;
        } else if (auto if_ = try_consume(TokenType::if_)) {
            try_consume(TokenType::open_paren, "Expected '('");
            auto stmt_if = m_allocator.alloc<NodeStmtIf>();
            if (auto expr = parse_expr()) {
                stmt_if->expr = expr.value();
                try_consume(TokenType::close_paren, "Expected ')'");
                if (auto scope = parse_scope()) {
                    stmt_if->scope = scope.value();
                    auto stmt = m_allocator.alloc<NodeStmt>();
                    stmt->var = stmt_if;
                    return stmt;
                } else {
                    std::cerr << "Invalid scope." << std::endl;
                    exit(EXIT_FAILURE);
                }
            } else {
                std::cerr << "Invalid expression." << std::endl;
                exit(EXIT_FAILURE);
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
