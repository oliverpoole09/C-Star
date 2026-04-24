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
node\_reassign
\\
node\_func\_def
\\
node\_return
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
\text{ReAssignNode}
\\
\text{FuncDefNode}
\\
\text{ReturnNode}
\\
\end{cases}
\\
\\
\text{ExitNode} &\to
\text{ExprNode}
\\
\\
\text{ReAssignNode} &\to
\begin{cases}
identifier\space\text{Token}
\\
\text{ExprNode}
\\
\end{cases}
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
\text{ExprData} &\to
\begin{cases}
\text{IntLiteralExpr}
\\
\text{VarExpr}
\\
\text{BinOpExpr}
\\
\text{FuncCallNode}
\\
\end{cases}
\\
\\
\text{BinOpExpr} &\to
\begin{cases}
\text{BinOp}
\\
left\text{ ExprNode}
\\
right\text{ ExprNode}
\\
\end{cases}
\\
\\
\text{FuncDefNode} &\to
\begin{cases}
\text{DataType}
\\
identifier\text{ Token}
\\
\text{Param } (up\ to\ 64)
\\
param\_count\ (int)
\\
\text{Node}
\\
body\_count\ (int)
\end{cases}
\\
\\
\text{FuncCallNode} &\to
\begin{cases}
identifier\text{ Token}
\\
\text{ExprNode } (up\ to\ 64)
\\
arg\_count\ (int)
\end{cases}
\\
\\
\text{Param} &\to
\begin{cases}
\text{DataType}
\\
identifier\text{ Token}
\\
\end{cases}
\\
\\
\text{ReturnNode} &\to
\text{ExprNode}
\\
\\
\text{VarExpr} &\to
identifier\text{ Token}
\\
\\
\text{IntLiteralExpr} &\to
value\space(\text{int})
\\
\\
\text{ExprType} &\to
\begin{cases}
expr\_int\_lit
\\
expr\_var
\\
expr\_binop
\\
expr\_func\_call
\\
\end{cases}
\\
\\
\text{DataType} &\to
\begin{cases}
data\_int
\end{cases}
\\
\\
\text{BinOp} &\to
\begin{cases}
op\_add
\\
op\_sub
\\
op\_mul
\\
op\_div
\end{cases}
\\
\\
\end{aligned}
$$