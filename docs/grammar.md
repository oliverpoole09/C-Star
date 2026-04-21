$$
\begin{aligned}
\text{Node} &\to
\begin{cases}
\text{NodeType}
\\
\text{NodeData}
\\
\end{cases}
\\
\\
\text{NodeType} &\to
\begin{cases}
node\_exit
\\
node\_var\_decl
\\
\end{cases}
\\
\\
\text{NodeData} &\to
\begin{cases}
\text{ExitNode}
\\
\text{VarDeclNode}
\\
\end{cases}
\\
\\
\text{ExitNode} &\to
\text{ExprNode}
\\
\\
\text{VarDeclNode} &\to
\begin{cases}
\text{DataType}
\\
identifier\text{ Token}
\\
\text{ExprNode}
\end{cases}
\\
\\
\text{ExprNode} &\to
\begin{cases}
\text{ExprType}
\\
\text{ExprData}
\\
\end{cases}
\\
\\
\text{ExprType} &\to
\begin{cases}
expr\_int\_lit
\\
expr\_var
\\
\end{cases}
\\
\\
\text{ExprData} &\to
\begin{cases}
\text{IntLiteralExpr}
\\
\text{VarExpr}
\\
\end{cases}
\\
\\
\text{IntLiteralExpr} &\to
value\space(\text{int})
\\
\\
\text{VarExpr} &\to
identifier\text{ Token}
\\
\\
\text{DataType} &\to
\begin{cases}
data\_int
\end{cases}
\end{aligned}
$$