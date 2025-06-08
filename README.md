#  :rocket:**Mini-Shell em C**

* Um interpretador de comandos simples, similar ao Bash, desenvolvido em C para a disciplina de Sistemas Operacionais.
---
### **Sobre o Projeto**

* Este projeto consiste na implementação de um shell interativo capaz de executar comandos, gerenciar processos e manipular a entrada e saída padrão, aplicando conceitos fundamentais de sistemas operacionais como fork(), execvp() e pipe().
---
### **Funcionalidades**
* **Execução de Comandos:** Suporte para comandos internos (cd, path) e externos (ls, gcc, etc.).
* **Redirecionamento de I/O:** Redirecionamento de entrada (<), saída (>) e anexação de saída (>>).
* **Pipelines:** Conexão da saída de um comando à entrada do próximo usando o operador pipe (|).
* **Execução em Background:** Execução de comandos em segundo plano com o operador (&), liberando o terminal para novos comandos.
* **Comandos Paralelos:** Suporte para múltiplos comandos na mesma linha, separados por &, para execução concorrente.
---
Compilar: ```gcc -o build/shell src/shell.c src/parser.c src/utils.c```
<br>Rodar: ```./build/shell```
