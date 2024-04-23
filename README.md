# Custom Shell Implementation
This repository contains the source code for a custom shell implementation in C. The shell provides various features and functionalities commonly found in Unix-like operating systems shells, including command execution, input/output redirection, piping, background process execution, conditional execution, and more.

# Features
- **Command Execution:** The shell allows users to execute commands provided through the terminal interface.
- **Input/Output Redirection:** Users can redirect input and output streams of commands using < and > operators, respectively.
- **Appending Output:** Additionally, the shell supports appending output to a file using the >> operator.
- **Piping:** Users can chain multiple commands together using the | operator to create pipelines for data communication between processes.
- **Background Process Execution:** Commands can be executed in the background by appending & to the end of the command.
- **Conditional Execution:** Conditional execution of commands based on the success or failure of previous commands is supported using && and || operators.
- **Sequential Execution:** Users can execute a sequence of commands separated by ; sequentially.

## Usage
To use the custom shell, follow these steps:

1. Clone the repository to your local machine.
2. Compile the source code using a C compiler, such as GCC: ```bash gcc -o custom_shell custom_shell.c```.
3. Run the compiled executable: ```bash ./custom_shell```.
4. Once the shell is running, you can enter commands in the terminal interface, and the shell will execute them accordingly.

## Additional Notes
- The shell provides enhanced functionality for command parsing, including support for tilde expansion (~), concatenation of files, and more.
- Users can create a new instance of the shell by entering the command newt, which spawns a new terminal window with the shell running.
