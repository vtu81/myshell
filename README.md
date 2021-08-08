# myshell

![visitors](https://visitor-badge.laobi.icu/badge?page_id=vtu.myshell)

[This repository](https://github.com/vtu81/myshell) contains a simplified Linux shell written in C, supporting commands:

* `cd`: change directory
* `pwd`: print working directory
* `time`: show current time
* `clr`: clear the screen
* `ls`: list files
* `umask`
* `set`
* `unset`
* `shift`
* `echo`
* `help`: use `help <command>` to check the command's usage
* `exec`: `exec <command>` executes the following command replacing the current process
* `jobs`
* `exit`
* `environ`
* Shell variables assignment like `variable=10`
* Basic pipeline of 2 commands split with '|'
* Redirection of stdin and stdout
* Execute external files

> The help books and code comments are written in simplified Chinese.

### Compile

```shell
g++ myshell.c -o myshell
```

### Run

```shell
./myshell
```

