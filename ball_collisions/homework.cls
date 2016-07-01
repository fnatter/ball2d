\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{homework}[1900/01/01]

\RequirePackage{ifthen}
\newboolean{redefineenumerate}

%% TODO:
%% homework.sty; homework.cls as wrapper for homework.sty:
% - option ``afour'' only in .cls !

%% declare options
\DeclareOption{math}{\AtEndOfClass{
\RequirePackage{amsmath}
%%
%% list for gathering conditions (systems of equations)
\newcounter{gatherconditionscnt}
\newcommand*{\makegatherconditionslabel}[1]{%
\addtocounter{gatherconditionscnt}{1}
\textbf{condition \arabic{gatherconditionscnt}}: [\texttt{#1}]}
\newenvironment{gatherconditions}{%
\begin{list}{}%
	{
	\setcounter{gatherconditionscnt}{0}
	\renewcommand{\makelabel}{\makegatherconditionslabel}}}{\end{list}}
%%
%% basic primitives
\newcommand*{\cvector}[3]{\ensuremath{\begin{bmatrix}#1\\#2\\#3\end{bmatrix}}}
\newcommand*{\cvectortwo}[2]{\ensuremath{\begin{bmatrix}#1 \\ #2\end{bmatrix}}}
\newcommand*{\rvector}[3]{\ensuremath{\begin{bmatrix}#1&#2&#3\end{bmatrix}}}
\newcommand*{\rvectortwo}[2]{\ensuremath{\begin{bmatrix}#1 & #2\end{bmatrix}}}
\newcommand*{\flatvector}[3]{\ensuremath{\rvector{#1}{#2}{#3}^T}}
\newcommand*{\flatvectortwo}[2]{\ensuremath{\rvectortwo{#1}{#2}^T}}
%%
%% additional function names
\newcommand{\sgn}{\ensuremath{\mathrm{sgn}}}
%% functional analysis
\newcommand*{\lefthandlimit}[3]{%
\ensuremath{\lim_{\overset{#1\to #2}{#1<#2}}#3}}
\newcommand*{\righthandlimit}[3]{%
\ensuremath{\lim_{\overset{#1\to #2}{#1>#2}}#3}}
\newcommand{\limhtozero}{\lim_{h\to 0}}
%% analytic geometry
\newcommand*{\intersectionbetween}[2]{\ensuremath{\vec{\mathrm{int}}_{#1, #2}}}
\newcommand*{\distancebetween}[2]{\ensuremath{\mathrm{dist}(#1, #2)}}
% note: if the first argument is sth like \vec{a}-\vec{b}, remember to put
% parentheses around it !
\newcommand*{\scalarprojectionof}[2]{\ensuremath{\mathrm{comp}_{#2} #1}}
\newcommand*{\deflinepara}[4]{\ensuremath{#1: \vec{x}=#2+#3#4}} %% 2D and 3D
\newcommand*{\defspherevec}[3]{\ensuremath{#1: \left(\vec{x}-#2\right)^2=#3}}
\newcommand*{\defcirclevec}[3]{\defspherevec{#1}{#2}{#3}}
\newcommand*{\defgeneric}[2]{\ensuremath{#1: #2}}
\newcommand*{\defhomogen}[2]{\ensuremath{#1: #2=0}}
\newcommand*{\defplane}[3]{\ensuremath{#1: \left(\vec{x}-#2\right)#3=0}}
\newcommand*{\defplanepara}[6]{\ensuremath{#1: \vec{x}=#2+#3#4+#5#6}}
\newcommand{\combinedwith}{\ensuremath{\,\wedge\,}}
\newcommand{\logicaland}{\combinedwith}
\newcommand{\logicalor}{\ensuremath{\,\vee\,}}
%% number-sets
\newcommand*{\typesetmathematicalset}[1]{\ensuremath{\mathbf{#1}}}
\newcommand{\isrealnumber}{\ensuremath{\in \typesetmathematicalset{R}}}
\newcommand{\isrationalnumber}{\ensuremath{\in \typesetmathematicalset{Q}}}
\newcommand{\isnaturalnumber}{\ensuremath{\in \typesetmathematicalset{N}}}
%% TODO: name for this
\newcommand{\iswholenumber}{\ensuremath{\in \typesetmathematicalset{Z}}}
\newcommand{\iscomplexnumber}{\ensuremath{\in \typesetmathematicalset{C}}}
%% TODO: rename this (hwrange vs. fxrange ?)
\newcommand*{\range}[2]{\ensuremath{_{#1}^{#2}}}
}
}

\DeclareOption{plainoldenumerate}{\setboolean{redefineenumerate}{true}}

\DeclareOption{afour}

\DeclareOption{german}{\AtEndOfClass{
\RequirePackage[german]{babel}
\newcommand*{\gquote}[1]{\glqq #1\grqq{}}
\newcommand*{\gquotewl}[2]{\glqq #1\grqq\ (Z. #2)}
}}

\DeclareOption{afour}{\AtEndOfClass{
%% define reasonable page dimensions for A4
\parindent=0cm
%% horizontal
\oddsidemargin=0cm
\textwidth=16cm
\marginparwidth=0cm
\marginparsep=0cm
%% vertical
\textheight=24cm
\topmargin=0cm
\headheight=0cm
\headsep=0cm
}}

\DeclareOption{terminator}{\AtEndOfClass{\RequirePackage[T1]{fontenc}}}

\DeclareOption{nativeinput}{\AtEndOfClass{\RequirePackage[latin1]{inputenc}}}

%% catch-all
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}

%% execute options
\ProcessOptions

%% Load 'article' class
\LoadClass[a4paper]{article}

%% ------------------- EOF(init) ---------------------------

%% redefine 'enumerate'
%% this is used to format index entries automatically (ensures consistency)
%% i.e.: \item[\hwindexformat{1,2}]
\newcommand*{\hwindexformat}[1]{\textbf{#1}}

\ifthenelse{\boolean{redefineenumerate}}{}{
\renewcommand{\theenumi}{\arabic{enumi}}
\renewcommand{\labelenumi}{\hwindexformat{\theenumi.}}
\renewcommand{\theenumii}{\alph{enumii}}
\renewcommand{\labelenumii}{\textbf{\theenumii.)}}
}

%% commands to generate title
\renewcommand{\maketitle}{%
\begin{center}\huge\@title, \@date\end{center}}
%% 'author' is optional argument
\newcommand*{\makebragbox}[3][Felix Natter]
{\title{#2}\date{#3}\author{#1}\maketitle}

\newcommand*{\TODO}[1][?]{\mbox{\textbf{TODO: #1}}\\}
\newcommand{\conclude}{$\Rightarrow\;$}
\newcommand*{\instruction}[1]{{\ttfamily #1}}

%% standard list for homework-tasks

\newcommand*{\homeworktasklabel}[1]{\mbox{\instruction{#1:}}\hfill}

% \newenvironment{homeworktask}{
% \begin{list}{%
% 	\renewcommand{\makelabel}{\homeworktasklabel}
% 	\setlength{\leftmargin}{0.2cm}
% 	\setlength{\rightmargin}{0cm}
% \end{list}
% }

%% some defaults which can easily be overridden
\AtBeginDocument{
\pagestyle{empty}
}





