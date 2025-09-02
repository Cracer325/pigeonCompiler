$$
\begin{align}
[\text{Prog}] &\to [\text{Stmt}]^* \\
[\text{Stmt}] &\to
\begin{cases}
\text{exit}([\text{Expr}]); \\
\text{let}\space\text{ident} = [\text{Expr}]; \\
\text{ident} = \text{[Expr]}; \\
\text{print([Expr]*);} \\
\end{cases} \\
[\text{Expr}] &\to
\begin{cases}
[\text{Term}] \\
[\text{BinExpr}]
\end{cases} \\
[\text{BinExpr}] &\to
\begin{cases}
[\text{Expr}] * [\text{Expr}] & \text{prec} = 1 \\
[\text{Expr}] / [\text{Expr}] & \text{prec} = 1 \\
[\text{Expr}] + [\text{Expr}] & \text{prec} = 0 \\
[\text{Expr}] - [\text{Expr}] & \text{prec} = 0 \\
\end{cases} \\
[\text{Term}] &\to
\begin{cases}
\text{int_lit} \\
\text{ident} \\
([\text{Expr}]) \\
-([\text{Expr}]) \\
\text{'char_lit'}\\
\end{cases}
\end{align}
$$