# Differentiator

## Installation 

```
git clone https://github.com/d3clane/Differentiator.git
make
```

## Description

This program can read math expression in human-readable common form and convert it into the tree. After that math expression could be differentiated and returned as another tree. There are different operations working with differentiating tree:
- Differentiate and print
- Set up Maclaurin Series up to nth term, print it and show up on the graph.
- Find derivative, print it and show it up on the graph

Also program dumps all stages of differentiating into tex file, shows graphs and series there. This tex is kind of parody on math books that often use such phrases as "It's obvious that", "Easy to see that" and so on.

Software used:
- C language, makefile
- GnuPlot to build graphs
- Luatex to auto generate pdf file
- Graphviz to build image of tree

## Reading math expression

I have used recursive descent algorithm in order to implement reading of math expression. This is not not the fastest or the best way to read math expressions, but it was a practise for my next project - [programming language](https://github.com/d3clane/ProgrammingLanguage).

Grammar for recursive descent:

```

G       ::= ADD_SUB '\0'
ADD_SUB ::= MUL_DIV {['+', '-'] MUL_DIV}*
MUL_DIV ::= POW {['*', '/'] POW}*
POW     ::= TRIG {['^'] TRIG}*
TRIG    ::= ['sin', 'cos', 'tan', 'cot', 'arctan', 'arccot', 'arcsin', 'arccos'] '(' ADD_SUB ')' | EXPR
EXPR    ::= '(' ADD_SUB ')' | ARG
ARG     ::= NUM | VAR
VAR     ::= ['a'-'z''A'-'Z''_']+['a'-'z' & 'A'-'Z' & '_' & '0'-'9']*
NUM     ::= ['0'-'9']+

```

Recursive descent is implemented in file [Differentiator/MathExpressionEquationRead.cpp](https://github.com/d3clane/Differentiator/blob/main/Differentiator/MathExpressionEquationRead.cpp)


## Building a tree

Recursive descent is creating different nodes and I can use it to differentiate math expression and build new tree.

Example:

Tree of math expression $\sin(x^2) + 2 \cdot x - (3^{{2 + 3 \cdot x}^{{21}^{x+2}}})^2 \cdot 16$:

![math expr tree](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/Tree.png)

## Differentiating the tree

Differentiating is done recursively, algorithm is pretty the same as people do it by themselves. There are three types of nodes - operation, values and variables. For each common operation (sin, cos, +, -, *, / and etc) there are built-in formulas of differentiating. For example $d(\sin(u)) = \cos(u) * d(u)$ And $d(u)$ is built recursively.

Previous tree differentiated:

![math expr tree diff](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/DiffTree.png)

## Simplifying the tree

As you may already mentioned tree seems to be really bulky and there could be unnecessary nodes. Tree looks like this because, for example, $d(x^n) = n * x^(n - 1)$ and my program doesn't calculate $n - 1$ on stage of differentiating. So, there's stage of simplifying for this cases.

Things that are simplified:
- Calculating constants which contain only number values. So, any subtree which consists only of constants would be merged in one node.

- Deleting neutral tokens. For example $x * 0 = 0$, $x + 0 = 0$, $x^0 = 1$, $\log_k(1) = 0$, etc.

Result after simplifying:

![math expr tree simple](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/SimpleTree.png)

## Derivative, Maclaurin Series

Calculating derivative and Maclaurin Series are pretty simple as soon I have already implemented differentiating function. All I needed was calculating tree with variables values from array. I already had this function and just used it.

## Graphs building 

Program builds tree and I have decided to dump it into gnu plot, call gnu plot to build it and then save. So, the main part - dumping. It's kind of obvious - tree dump in infix order. Gnu plot building result:

![MainGraph](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/MainGraph.png)

![Tangent](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/Tangent.png)

![Maclaurin](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/Maclaurin.png)

## Tex dump

Building tex file using [tex start asset](Differentiator/latexStartAsset.txt) and [tex end asset](Differentiator/latexEndAsset.txt). After that program call luatex and compile [pdf](Differentiator/PHD.pdf). In latex file there are much more graphs and also parody on research paper provided.

Some formulas could be really big and overstep the boundaries of latex file. Because of that I have coded mechanism of variable replacement that finds too long parts of formula and redefines them with variables. There's an example of this behaviour:

![variable replacement](https://github.com/d3clane/Differentiator/blob/main/ReadmeAssets/imgs/Replacement.png)

