$$
\begin{aligned}
\\
[\text{prog}] &\to 
[\text{stmt}]^*
\\
[\text{stmt}] &\to
\begin{cases}
\text{exit}([\text{expr}]);
\\
\text{dat_type} \space \text{ident} = [\text{expr}];
\end{cases}
\\
[\text{exit}] &\to exit(\text{[expr]});
\\
[\text{expr}] &\to
\begin{cases}
\text{int_lit}
\\
\text{ident}
\end{cases}
\\
[\text{dat_type}] &\to
\begin{cases}
\text{byte}
\\
\text{int}
\\
\text{long}
\\
\text{float}
\\
\text{double}
\\
\text{bool}
\\
\text{char}
\\
\text{string}
\\
\end{cases}
\end{aligned}
$$