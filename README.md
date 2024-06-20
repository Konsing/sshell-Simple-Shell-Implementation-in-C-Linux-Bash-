# sshell: Simple Shell Implementation in C

This project implements a simple shell called `sshell` in C. The goal is to understand important UNIX system calls by implementing a command-line interpreter that accepts user input and executes commands. The shell supports basic functionalities such as command execution, built-in commands, output redirection, and piping.

## Features
- Execution of user-supplied commands with optional arguments.
- Built-in commands: `exit`, `cd`, `pwd`.
- Standard output redirection to files.
- Command composition via piping.
- Output redirection appending.
- Simple `ls`-like built-in command (`sls`).

## Constraints
- Maximum command line length: 512 characters.
- Maximum number of non-null arguments: 16.
- Maximum length of individual tokens: 32 characters.
- Programs should be searched according to the `$PATH` environment variable.
- Written in C, compiled with GCC, and only use the GNU C Library (libc).

## File Structure
- `sshell.c`: Main implementation of the simple shell.
- `Makefile`: Build instructions for the project.

## Setup Instructions

```sh
1. **Clone the Repository**:
   git clone <repository-url>
   cd <repository-directory>

2. **Build the Project**:
   Use the provided Makefile to build the project.
   make

3. **Run the Shell**:
   ./sshell

## Setup Instructions

```markdown
# Simple Shell

This project is a simple shell program implemented in C. The shell can execute basic commands, handle input/output redirection, background execution, and command piping. It also supports built-in commands such as `pwd` and `cd`.

## Features

- **Basic Command Execution**: Execute any regular command available on the system.
- **Built-in Commands**: Includes `pwd` and `cd` commands.
- **Piping**: Supports piping (`|`) between commands.
- **Input/Output Redirection**: Supports output redirection (`>` and `>>`).
- **Background Execution**: Execute commands in the background using `&`.
- **Error Handling**: Provides error messages for invalid commands, missing files, etc.

## Files

- `shell.c`: The main implementation file for the shell.
- `Makefile`: Contains the build instructions for the project.

## Installation

To build the shell, run the following command:

```sh
make
```

This will compile the `shell.c` file and create an executable named `sshell`.

## Usage

To start the shell, run:

```sh
./sshell
```

You will be presented with a prompt where you can enter your commands.

### Examples

1. **Basic Command**:
    ```sh
    sshell@ucd$ ls
    ```

2. **Change Directory**:
    ```sh
    sshell@ucd$ cd /path/to/directory
    ```

3. **Print Working Directory**:
    ```sh
    sshell@ucd$ pwd
    ```

4. **Output Redirection**:
    ```sh
    sshell@ucd$ ls > output.txt
    ```

5. **Append Output Redirection**:
    ```sh
    sshell@ucd$ echo "Hello World" >> output.txt
    ```

6. **Background Execution**:
    ```sh
    sshell@ucd$ sleep 10 &
    ```

7. **Piping**:
    ```sh
    sshell@ucd$ ls | grep shell
    ```

## Error Handling

The shell provides detailed error messages for various invalid commands and syntax errors, such as:

- Command exceeding the maximum length.
- Token or argument exceeding the maximum length.
- More than 16 non-null arguments.
- Misplaced output redirection.
- Missing directory for `cd`.
- Missing file name for `cat`.
- Misplaced background sign.