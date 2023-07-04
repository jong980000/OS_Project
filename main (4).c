#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#define DEFAULT printf("\x1b[0m")
#define MAX_BUFFER 512
#define MAX_LENGTH 200
#define MAX_DIR 50
#define MAX_NAME 20
#define MAX_THREAD 50

// User
typedef struct tagUserNode{
    char name[MAX_NAME];
    char dir[MAX_DIR];
    int UID;
    int GID;
    int year;
    int month;
    int wday;
    int day;
    int hour;
    int minute;
    int sec;
    struct tagUserNode* LinkNode;
}UserNode;

typedef struct tagUser{
    int topUID;
    int topGID;
    UserNode* head;
    UserNode* tail;
    UserNode* current;
}UserList;


//Directory
typedef struct tagTreeNode{
    char name[MAX_NAME];
    char type;
    int mode;
    int permission[9];
    int SIZE;
    int UID;
    int GID;
    int month;
    int day;
    int hour;
    int minute;
    struct tagTreeNode* Parent;
    struct tagTreeNode* LeftChild;
    struct tagTreeNode* RightSibling;
}TreeNode;

typedef struct tagDirectoryTree{
    TreeNode* root;
    TreeNode* current;
}DirectoryTree;


//stack using linked list
typedef struct tagStackNode{
    char name[MAX_NAME];
    struct tagStackNode* LinkNode;
}StackNode;


typedef struct tagStack{
    StackNode* TopNode;
}Stack;

typedef struct tagThread{
    DirectoryTree *dirTree;
    char *cmd;
    char type;
}ThreadArg;

time_t ltime;
struct tm *today;


int mkdir_(DirectoryTree* dirTree, char* cmd);
int rm(DirectoryTree* dirTree, char* cmd);
int cd(DirectoryTree* dirTree, char* cmd);
int pwd(DirectoryTree* dirTree, Stack* dirStack, char* cmd);
int cat(DirectoryTree* dirTree, char* cmd);
void PrintHelp();

void PrintChmodHelp();
void PrintChmodError(int err_code,char* str);
int chmod_(DirectoryTree *dirTree, char *cmd);
int chown_(DirectoryTree* dirTree, char* cmd);
int find(DirectoryTree* dirTree, char* cmd);
void Instruction(DirectoryTree* dirTree, char* cmd);
void PrintHead(DirectoryTree* dirTree, Stack* dirStack);

//grep
char* TrimString(char *str);
void TextError(int a, char *str);
void grep(char *searching_word, char *f_name);
void grep_n(char *searching_word,char *f_name);
//directory

//utility
int Mode2Permission(TreeNode* dirNode);
void PrintPermission(TreeNode* dirNode);
void DestroyNode(TreeNode* dirNode);
void DestroyDir(TreeNode* dirNode);
TreeNode* IsExistDir(DirectoryTree* dirTree, char* dirName, char type);
char* GetDir(char* dirPath);

//save & load
void getPath(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack);
void WriteNode(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack);
void SaveDir(DirectoryTree* dirTree, Stack* dirStack);
int ReadNode(DirectoryTree* dirTree, char* tmp);
DirectoryTree* LoadDir();

//mycp
int mycp(DirectoryTree* dirTree,char* sName,char* oName);

//mkdir
DirectoryTree* InitializeTree();
int MakeDir(DirectoryTree* dirTree, char* dirName, char type);
void *t_MakeDir(void *arg);
// rm
int RemoveDir(DirectoryTree* dirTree, char* dirName);
//cd
int MoveCurrent(DirectoryTree* dirTree, char* dirPath);
int MovePath(DirectoryTree* dirTree, char* dirPath);
//pwd
int PrintPath(DirectoryTree* dirTree, Stack* dirStack);
//ls
void ls(DirectoryTree* dirTree);
void ls_a(DirectoryTree* dirTree);
void ls_l(DirectoryTree* dirTree);
void ls_al(DirectoryTree* dirTree);

//cat
int Concatenate(DirectoryTree* dirTree, char* fName, int o);
//chmod
int ChangeMode(DirectoryTree* dirTree, int mode, char* dirName);
void ChangeModeAll(TreeNode* dirNode, int mode);
//chown
int ChangeOwner(DirectoryTree* dirTree, char* userName, char* dirName);
void ChangeOwnerAll(TreeNode* dirNode, char* userName);
//find
int ReadDir(DirectoryTree* dirTree, char* tmp, char* dirName,int o);
void FindDir(DirectoryTree* dirTree, char* dirName, int o);

//user
void WriteUser(UserList* userList, UserNode* userNode);
void SaveUserList(UserList* userList);
int ReadUser(UserList* userList, char* tmp);
UserList* LoadUserList();
UserNode* IsExistUser(UserList* userList, char* userName);

int CheckId(TreeNode* dirNode,char o,int i);
int HasPermission(TreeNode *dirNode, char o);
void Login(UserList* userList, DirectoryTree* dirTree);
//stack
int IsEmpty(Stack* dirStack);
Stack* InitializeStack();
int Push(Stack* dirStack, char* dirName);
char* Pop(Stack* dirStack);


//global variable
DirectoryTree* g_linux;
Stack* g_dir_stack;
UserList* g_user_list;
FILE* g_dir;
FILE* g_user;

int main(){
    char cmd[50];
    g_linux = LoadDir();
    g_user_list = LoadUserList();
    g_dir_stack = InitializeStack();

    Login(g_user_list, g_linux);
    SaveUserList(g_user_list);

    while(1){
        PrintHead(g_linux, g_dir_stack);
        fgets(cmd, sizeof(cmd), stdin);
        cmd[strlen(cmd)-1] = '\0';
        Instruction(g_linux, cmd);
    }
    return 0;
}

int mkdir_(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    pthread_t t_id[MAX_THREAD];
    ThreadArg t_args[MAX_THREAD];
    int t_cnt = 0;
    int val;
    int tmpMode;
    if(cmd == NULL){
        printf("mkdir: 잘못된 연산자\n");
        printf("Try 'mkdir --help' for more information.\n");
        return -1;
    }

    tmpNode = dirTree->current;
    if(cmd[0] == '-'){
        if(strcmp(cmd, "-p") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                printf("mkdir: 잘못된 연산자\n");
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
            if(strncmp(str, "/", 1) == 0){
                dirTree->current = dirTree->root;
            }
            str = strtok(str, "/");
            while(str != NULL){
                val = MoveCurrent(dirTree, str);
                if(val != 0){
                    MakeDir(dirTree, str, 'd');
                    MoveCurrent(dirTree, str);
                }
                str = strtok(NULL, "/");
            }
            dirTree->current = tmpNode;
        }
        else if(strcmp(cmd, "-m") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                printf("mkdir: 잘못된 연산자\n");
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
            if(str[0]-'0'<8 && str[1]-'0'<8 && str[2]-'0'<8 && strlen(str)==3){
                tmpMode = atoi(str);
                str = strtok(NULL, " ");
                if(str == NULL){
                    printf("mkdir: 잘못된 연산자\n");
                    printf("Try 'mkdir --help' for more information.\n");
                    return -1;
                }
                val = MakeDir(dirTree, str, 'd');
                if(val == 0){
                    tmpNode = IsExistDir(dirTree, str, 'd');
                    tmpNode->mode = tmpMode;
                    Mode2Permission(tmpNode);
                }
            }
            else{
                printf("mkdir: 잘못된 모드: '%s'\n", str);
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
        }
        else{
            str = strtok(cmd, "-");
            if(str == NULL){
                printf("mkdir: 잘못된 연산자\n");
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
            else{
                printf("mkdir: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'mkdir --help' for more information.\n");
                return -1;
            }
        }
    }
    else{
        str = strtok(NULL, " ");
        if (str == NULL)
        {
            strncpy(tmp, cmd, MAX_DIR);
            
            if(strstr(cmd, "/") == NULL){
                MakeDir(dirTree, cmd, 'd');
                return 0;
            }
            else{
                strncpy(tmp2, GetDir(cmd), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if(val != 0){
                    printf("mkdir: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while(str != NULL){
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                MakeDir(dirTree, tmp3 , 'd');
                dirTree->current = tmpNode;
            }
        }
        else{
            t_args[t_cnt].dirTree = dirTree;
            t_args[t_cnt].cmd = cmd;
            t_args[t_cnt++].type = 'd';
            while(str != NULL){
                t_args[t_cnt].dirTree = dirTree;
                t_args[t_cnt].cmd = str;
                t_args[t_cnt++].type = 'd';
                str = strtok(NULL, " ");
            }
            for (int i = 0; i < t_cnt;i++){
                pthread_create(&t_id[i], NULL, t_MakeDir, (void *)&t_args[i]);
                pthread_join(t_id[i], NULL);
            }
        }
    }
    return 0;
}

int chown_(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    UserNode* tmpUser = NULL;
    char* str;
    char tmp[MAX_NAME];

    if(cmd == NULL){
        printf("chown: 잘못된 연산자\n");
        printf("Try 'chown --help' for more information.\n");
        return -1;
    }
    if(cmd[0] == '-'){
        if(strcmp(cmd, "-R") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                printf("chown: 잘못된 연산자\n");
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            tmpUser = IsExistUser(g_user_list, str);
            if(tmpUser != NULL){
                strncpy(tmp, str, MAX_NAME);
            }
            else{
                printf("chown: 잘못된 사용자: '%s'\n", str);
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            str = strtok(NULL, " ");
            if(str == NULL){
                printf("chown: 잘못된 연산자\n");
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            tmpNode = IsExistDir(dirTree, str, 'd');
            if(tmpNode != NULL){
                if(tmpNode->LeftChild == NULL)
                    ChangeOwner(dirTree, tmp, str);
                else{
                    ChangeOwner(dirTree, tmp, str);
                    ChangeOwnerAll(tmpNode->LeftChild, tmp);
                }
            }
            else{
                printf("chown: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                return -1;
            }
        }
        else if(strcmp(cmd, "--help") == 0){
            printf("사용법: chown [옵션]... [소유자]... 파일...\n");
            printf("  Change the owner and/or group of each FILE to OWNER and/or GROUP.\n\n");
            printf("  Options:\n");
            printf("    -R, --recursive\t change files and directories recursively\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else{
            str = strtok(cmd, "-");
            if(str == NULL){
                printf("chown: 잘못된 연산자\n");
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
            else{
                printf("chown: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'chown --help' for more information.\n");
                return -1;
            }
        }
    }
    else{
        strncpy(tmp, cmd, MAX_NAME);
        str = strtok(NULL, " ");
        if(str == NULL){
            printf("chown: 잘못된 연산자\n");
            printf("Try 'chown --help' for more information.\n");
            return -1;
        }
        else{
            ChangeOwner(dirTree, tmp, str);
        }
    }
    return 0;
}

int rm(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* currentNode = NULL;
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    char path[MAX_DIR] = "./";
    int val;

    if(cmd == NULL){
        printf("rm: 잘못된 연산자\n");
        printf("Try 'rm --help' for more information.\n");
        return -1;
    }
    currentNode = dirTree->current;
    if(cmd[0] == '-'){
        if(strcmp(cmd, "-r") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                printf("rm: 잘못된 연산자\n");
                printf("Try 'rm --help' for more information.\n");

                return -1;
            }
            strncpy(tmp, str, MAX_DIR);
            if(strstr(str, "/") == NULL){
                tmpNode = IsExistDir(dirTree, str, 'd');
                if(tmpNode == NULL){
                    printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
                else{
                    if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                        printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", str);
                        return -1;
                    }
                    RemoveDir(dirTree, str);
                }
            }
            else{
                strncpy(tmp2, GetDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if(val != 0){
                    printf("rm: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while(str != NULL){
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                if(tmpNode == NULL){
                    printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else{
                    if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                        printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", tmp3);
                        dirTree->current = currentNode;
                        return -1;
                    }
                    RemoveDir(dirTree, tmp3);
                }
                dirTree->current = currentNode;
            }
        }
        else if(strcmp(cmd, "-f") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                return -1;
            }
            strncpy(tmp, str, MAX_DIR);
            if(strstr(str, "/") == NULL){
                tmpNode = IsExistDir(dirTree, str, 'f');
                tmpNode2 = IsExistDir(dirTree, str, 'd');

                if(tmpNode2 != NULL){
                    return -1;
                }
                if(tmpNode == NULL){
                    return -1;
                }
                else{
                    if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                        return -1;
                    }
                    RemoveDir(dirTree, str);
                }
            }
            else{
                strncpy(tmp2, GetDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if(val != 0){
                    return -1;
                }
                str = strtok(tmp, "/");
                while(str != NULL){
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'f');
                tmpNode2 = IsExistDir(dirTree, tmp3, 'd');

                if(tmpNode2 != NULL){
                    dirTree->current = currentNode;
                    return -1;
                }
                if(tmpNode == NULL){
                    dirTree->current = currentNode;
                    return -1;
                }
                else{
                    if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                        dirTree->current = currentNode;
                        return -1;
                    }
                    RemoveDir(dirTree, tmp3);
                }
                dirTree->current = currentNode;
            }
        }
        else if(strcmp(cmd, "-rf") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                return -1;
            }
            strncpy(tmp, str, MAX_DIR);
            if(strstr(str, "/") == NULL){
                tmpNode = IsExistDir(dirTree, str, 'd');
                if(tmpNode == NULL){
                    return -1;
                }
                else{
                    if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                        return -1;
                    }
                    RemoveDir(dirTree, str);
                }
            }
            else{
                strncpy(tmp2, GetDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if(val != 0){
                    return -1;
                }
                str = strtok(tmp, "/");
                while(str != NULL){
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                if(tmpNode == NULL){
                    dirTree->current = currentNode;
                    return -1;
                }
                else{
                    if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                        dirTree->current = currentNode;
                        return -1;
                    }
                    RemoveDir(dirTree, tmp3);
                }
                dirTree->current = currentNode;
            }
        }
        else if(strcmp(cmd, "--help") == 0){
            printf("사용법: rm [<옵션>]... [<파일>]...\n");
            printf("  Remove (unlink) the FILE(s).\n\n");
            printf("  Options:\n");
            printf("    -f, --force    \t ignore nonexistent files and arguments, never prompt\n");
            printf("    -r, --recursive\t remove directories and their contents recursively\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else{
            str = strtok(cmd, "-");
            if(str == NULL){
                printf("rm: 잘못된 연산자\n");
                printf("Try 'rm --help' for more information.\n");
                return -1;
            }
            else{
                printf("rm: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'rm --help' for more information.\n");
                return -1;
            }
        }
    }
    else{
        strncpy(tmp, cmd, MAX_DIR);
        if(strstr(cmd, "/") == NULL){
            tmpNode = IsExistDir(dirTree, cmd, 'f');
            tmpNode2 = IsExistDir(dirTree, cmd, 'd');

            if(tmpNode2 != NULL){
                printf("rm:'%s'를 지울 수 없음: 디렉터리입니다\n", cmd);
                return -1;
            }
            if(tmpNode == NULL){
                printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", cmd);
                return -1;
            }
            else{
                if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                    printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", cmd);
                    return -1;
                }
                RemoveDir(dirTree, cmd);
                strcat(path, cmd);
                remove(path);
            }
        }
        else{
            strncpy(tmp2, GetDir(cmd), MAX_DIR);
            val = MovePath(dirTree, tmp2);
            if(val != 0){
                printf("rm: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                return -1;
            }
            str = strtok(tmp, "/");
            while(str != NULL){
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            tmpNode = IsExistDir(dirTree, tmp3, 'f');
            tmpNode2 = IsExistDir(dirTree, tmp3, 'd');

            if(tmpNode2 != NULL){
                printf("rm:'%s'를 지울 수 없음: 디렉터리입니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            if(tmpNode == NULL){
                printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else{
                if(HasPermission(dirTree->current, 'w') != 0 || HasPermission(tmpNode, 'w') != 0){
                    printf("rm: '%s'디렉터리 또는 파일을 지울 수 없습니다: 허가거부\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                RemoveDir(dirTree, tmp3);
                strcat(path,tmp3);
                remove(path);
            }
            dirTree->current = currentNode;
        }
    }
    return 0;
}

int pwd(DirectoryTree* dirTree, Stack* dirStack, char* cmd)
{
    char* str = NULL;
    if(cmd == NULL){
        PrintPath(dirTree, dirStack);
    }
    else if(cmd[0] == '-'){
        if(strcmp(cmd, "--help") == 0){
            printf("사용법: pwd\n");
            printf("  Print the name of the current working directory.\n\n");
            printf("  Options:\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else{
            str = strtok(cmd, "-");
            if(str == NULL){
                printf("pwd: 잘못된 연산자\n");
                printf("Try 'pwd --help' for more information.\n");
                return -1;
            }
            else{
            printf("pwd: 부적절한 옵션 -- '%s'\n", str);
            printf("Try 'pwd --help' for more information.\n");
            return -1;
            }
        }
    }

    return 0;
}

int cat(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* currentNode = NULL;
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    int val;
    /*
        cat0: write, EOF to save
        cat1: read
        cat2: read w/ line number
    */
    if(cmd == NULL){
        printf("cat: 잘못된 연산자\n");
        return -1;
    }
    currentNode = dirTree->current;

    if(strcmp(cmd, ">") == 0){  // > 옵션 사용 했을 때
        str = strtok(NULL, " ");
        if(str == NULL){
            printf("cat: 잘못된 연산자\n");
            printf("Try 'cat --help' for more information.\n");
            return -1;
        }
        strncpy(tmp, str, MAX_DIR);
        if(strstr(str, "/") == NULL){
            if(HasPermission(dirTree->current, 'w') != 0){ // 쓰기권한인지 확인
                printf("cat: '%s'파일을 만들 수 없음: 권한없음\n", dirTree->current->name);
                return -1;
            }
            tmpNode = IsExistDir(dirTree, str, 'd'); //쓰기권한이 있으면 IsExistDIR실행
            if(tmpNode != NULL){
                printf("cat: '%s': 디렉터리입니다\n", str);
                return -1;
            }
            else{
            Concatenate(dirTree, str, 0);
            }
        }
        else{
            strncpy(tmp2, GetDir(str), MAX_DIR);
            val = MovePath(dirTree, tmp2);
            if(val != 0){
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                return -1;
            }
            str = strtok(tmp, "/");
            while(str != NULL){
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            if(HasPermission(dirTree->current, 'w') != 0){
                printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                dirTree->current = currentNode;
                return -1;
            }
            tmpNode = IsExistDir(dirTree, tmp3, 'd');
            if(tmpNode != NULL){
                printf("cat: '%s': 디렉터리입니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else{
            Concatenate(dirTree, tmp3, 0);
            }
            dirTree->current = currentNode;
        }
        return 0;
    }
    else if(cmd[0] == '-'){
        if(strcmp(cmd, "-n")== 0){
            str = strtok(NULL, " ");
            strncpy(tmp, str, MAX_DIR);
            if(strstr(str, "/") == NULL){
                if(HasPermission(dirTree->current, 'w') != 0){
                    printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                    return -1;
                }
                tmpNode = IsExistDir(dirTree, str, 'd');
                tmpNode2 = IsExistDir(dirTree, str, 'f');

                if(tmpNode == NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
                else if(tmpNode != NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 디렉터리입니다\n", str);
                    return -1;
                }
                else if(tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0){
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    return -1;
                }
                else{
                Concatenate(dirTree, str, 2);
                }
            }
            else{
                strncpy(tmp2, GetDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if(val != 0){
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while(str != NULL){
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                tmpNode2 = IsExistDir(dirTree, tmp3, 'f');

                if(tmpNode == NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if(tmpNode != NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 디렉터리입니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if(tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0){
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    dirTree->current = currentNode;
                    return -1;
                }
                else{
                Concatenate(dirTree, tmp3, 2);
                }
                dirTree->current = currentNode;
            }
        }
        else if(strcmp(cmd, "-b")== 0){
            str = strtok(NULL, " ");
            strncpy(tmp, str, MAX_DIR);
            if(strstr(str, "/") == NULL){
                if(HasPermission(dirTree->current, 'w') != 0){
                    printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                    return -1;
                }
                tmpNode = IsExistDir(dirTree, str, 'd');
                tmpNode2 = IsExistDir(dirTree, str, 'f');
                if(tmpNode == NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
                    return -1;
                }
                else if(tmpNode != NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 디렉터리입니다\n", str);
                    return -1;
                }
                else if(tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0){
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    return -1;
                }
                else{
                Concatenate(dirTree, str, 3);
                }
            }
            else{
                strncpy(tmp2, GetDir(str), MAX_DIR);
                val = MovePath(dirTree, tmp2);
                if(val != 0){
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                    return -1;
                }
                str = strtok(tmp, "/");
                while(str != NULL){
                    strncpy(tmp3, str, MAX_NAME);
                    str = strtok(NULL, "/");
                }
                tmpNode = IsExistDir(dirTree, tmp3, 'd');
                tmpNode2 = IsExistDir(dirTree, tmp3, 'f');
                if(tmpNode == NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if(tmpNode != NULL && tmpNode2 == NULL){
                    printf("cat: '%s': 디렉터리입니다\n", tmp3);
                    dirTree->current = currentNode;
                    return -1;
                }
                else if(tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0){
                    printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                    dirTree->current = currentNode;
                    return -1;
                }
                else{
                Concatenate(dirTree, tmp3, 3);
                }
                dirTree->current = currentNode;
            }
        }
        else if(strcmp(cmd, "--help") == 0){
            printf("사용법: cat [<옵션>]... [<파일>]...\n");
            printf("  FILE(들)을 합쳐서 표준 출력으로 보낸다.\n\n");
            printf("  Options:\n");
            printf("    -n, --number         \t number all output line\n");
            printf("    -b, --number-nonblank\t number nonempty output line\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else{
            str = strtok(cmd, "-");
            if(str == NULL){
                printf("cat: 잘못된 연산자\n");
                printf("Try 'cat --help' for more information.\n");
                return -1;
            }
            else{
                printf("cat: 부적절한 옵션 -- '%s'\n", str);
                printf("Try 'cat --help' for more information.\n");
                return -1;
            }
        }
    }
    else{
        strncpy(tmp, cmd, MAX_DIR);
        if(strstr(cmd, "/") == NULL){
            // printf("%s\n",cmd);
            if (HasPermission(dirTree->current, 'w') != 0)
            {
                printf("cat: '%s'파일을 만들 수 없음: 허가거부\n", dirTree->current->name);
                return -1;
            }
            tmpNode = IsExistDir(dirTree, cmd, 'd');
            tmpNode2 = IsExistDir(dirTree, cmd, 'f');
            if(tmpNode == NULL && tmpNode2 == NULL){
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", cmd);
                return -1;
            }
            else if(tmpNode != NULL && tmpNode2 == NULL){
                printf("cat: '%s': 디렉터리입니다\n", cmd);
                return -1;
            }
            else if(tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0){
                printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                return -1;
            }
            else{
            Concatenate(dirTree, cmd, 1);
            }

        }
        else{
            strncpy(tmp2, GetDir(cmd), MAX_DIR);
            val = MovePath(dirTree, tmp2);
            if(val != 0){
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
                return -1;
            }
            str = strtok(tmp, "/");
            while(str != NULL){
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            tmpNode = IsExistDir(dirTree, tmp3, 'd');
            tmpNode2 = IsExistDir(dirTree, tmp3, 'f');
            if(tmpNode == NULL && tmpNode2 == NULL){
                printf("cat: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else if(tmpNode != NULL && tmpNode2 == NULL){
                printf("cat: '%s': 디렉터리입니다\n", tmp3);
                dirTree->current = currentNode;
                return -1;
            }
            else if(tmpNode2 != NULL && HasPermission(tmpNode2, 'r') != 0){
                printf("cat: '%s'파일을 열 수 없음: 허가거부\n", tmpNode2->name);
                dirTree->current = currentNode;
                return -1;
            }
            else{
            Concatenate(dirTree, tmp3, 1);
            }
            dirTree->current = currentNode;
        }
    }
    return 1;
}

//chmod
int ChangeMode(DirectoryTree* dirTree, int mode, char* dirName)
{
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    tmpNode = IsExistDir(dirTree, dirName, 'd');
    tmpNode2 = IsExistDir(dirTree, dirName, 'f');

    if(tmpNode != NULL){
        if(HasPermission(tmpNode, 'w') != 0){
            printf("chmod: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpNode->mode = mode;
        Mode2Permission(tmpNode);
    }
    else if(tmpNode2 != NULL){
        if(HasPermission(tmpNode2, 'w') != 0){
            printf("chmod: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpNode2->mode = mode;
        Mode2Permission(tmpNode2);
    }
    else{
        printf("chmod: '%s에 접근할 수 없습니다: 그런 파일이나 디렉터리가 없습니다\n", dirName);
        return -1;
    }
    return 0;
}

void ChangeModeAll(TreeNode* dirNode, int mode)
{
    if(dirNode->RightSibling != NULL){
        ChangeModeAll(dirNode->RightSibling, mode);
    }
    if(dirNode->LeftChild != NULL){
        ChangeModeAll(dirNode->LeftChild, mode);
    }
    dirNode->mode = mode;
    Mode2Permission(dirNode);
}

//chown
int ChangeOwner(DirectoryTree* dirTree, char* userName, char* dirName)
{
    TreeNode* tmpNode = NULL;
    TreeNode* tmpNode2 = NULL;
    UserNode* tmpUser = NULL;
    tmpNode = IsExistDir(dirTree, dirName, 'd');
    tmpNode2 = IsExistDir(dirTree, dirName, 'f');

    if(tmpNode != NULL){
        if(HasPermission(tmpNode, 'w') != 0){
            printf("chown: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpUser = IsExistUser(g_user_list, userName);
        if(tmpUser != NULL){
            tmpNode->UID = tmpUser->UID;
            tmpNode->GID = tmpUser->GID;
        }
        else{
            printf("chown: 잘못된 사용자: '%s'\n", userName);
            printf("Try 'chown --help' for more information.\n");
            return -1;
        }
    }
    else if(tmpNode2 != NULL){
        if(HasPermission(tmpNode2, 'w') != 0){
            printf("chown: '%s'파일을 수정할 수 없음: 허가거부\n", dirName);
            return -1;
        }
        tmpUser = IsExistUser(g_user_list, userName);
        if(tmpUser != NULL){
            tmpNode2->UID = tmpUser->UID;
            tmpNode2->GID = tmpUser->GID;
        }
        else{
            printf("chown: 잘못된 사용자: '%s'\n", userName);
            printf("Try 'chown --help' for more information.\n");
            return -1;
        }
    }
    else{
        printf("chown: '%s'에 접근할 수 없습니다: 그런 파일이나 디렉터리가 없습니다\n", dirName);
        return -1;
    }
    return 0;
}

void ChangeOwnerAll(TreeNode* dirNode, char* userName)
{
    UserNode* tmpUser = NULL;
    tmpUser = IsExistUser(g_user_list, userName);

    if(dirNode->RightSibling != NULL){
        ChangeOwnerAll(dirNode->RightSibling, userName);
    }
    if(dirNode->LeftChild != NULL){
        ChangeOwnerAll(dirNode->LeftChild, userName);
    }
    dirNode->UID = tmpUser->UID;
    dirNode->GID = tmpUser->GID;
}

void PrintChmodError(int err_code,char* str){
    if(err_code==1){
        printf("chmod: '%s': 그런 파일이나 디렉터리가 없습니다\n", str);
        return;
    }
    if(err_code==0){
        printf("chmod: 잘못된 연산자\n");
    }else if(err_code==2){
        printf("chmod: 잘못된 모드: '%s'\n", str);
    }else{
        printf("chmod: 부적절한 옵션 -- '%s'\n", str);
    }
    printf("Try 'chmod --help' for more information.\n");
    return;
}

void PrintChmodHelp(){
    printf("사용법: chmod [옵션]... 8진수-MODE... 디렉터리...\n");
    printf("  Change the mode of each FILE to MODE.\n\n");
    printf("  Options:\n");
    printf("    -R, --recursive\t change files and directories recursively\n");
    printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
}

int chmod_(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    char* str;
    int tmp;

    if(cmd == NULL){
        PrintChmodError(0, NULL);
        return -1;
    }
    if(cmd[0] == '-'){
        if(strcmp(cmd, "-R") == 0){
            str = strtok(NULL, " ");
            if(str == NULL){
                PrintChmodError(0, NULL);
                return -1;
            }
            if(str[0]-'0'<8 && str[1]-'0'<8 && str[2]-'0'<8 && strlen(str)==3){
                tmp = atoi(str);
                str = strtok(NULL, " ");
                if(str == NULL){
                    PrintChmodError(0, NULL);
                    return -1;
                }
                tmpNode = IsExistDir(dirTree, str, 'd');
                if(tmpNode != NULL){
                    if(tmpNode->LeftChild == NULL)
                        ChangeMode(dirTree, tmp, str);
                    else{
                        ChangeMode(dirTree, tmp, str);
                        ChangeModeAll(tmpNode->LeftChild, tmp);
                    }
                }
                else{
                    PrintChmodError(1, str);
                    return -1;
                }
            }
            else{
                PrintChmodError(2, str); // 모드 숫자 에러
                return -1;
            }
        }
        else if(strcmp(cmd, "--help") == 0){
            PrintChmodHelp();
            return -1;
        }
        else{
            str = strtok(cmd, "-");
            if(str == NULL){
                PrintChmodError(0, NULL);
                return -1;
            }
            else{
                PrintChmodError(3, str);
                return -1;
            }
        }
    }
    else{
        if(cmd[0]-'0'<8 && cmd[1]-'0'<8 && cmd[2]-'0'<8 && strlen(cmd)==3){
            tmp = atoi(cmd);
            str = strtok(NULL, " ");
            if(str == NULL){
                PrintChmodError(0, NULL);
                return -1;
            }
            ChangeMode(dirTree, tmp, str);
        }
        else{
            PrintChmodError(2, cmd);//모드 숫자 에러
            return -1;
        }
    }
    return 0;
}

//find
int ReadDir(DirectoryTree* dirTree, char* tmp, char* dirName, int o)
{
    char* str;
    char str2[MAX_NAME];
    if(o == 0){
        str = strtok(tmp, " ");
        strcpy(str2, str);
        for(int i=0;i<10;i++){
            str = strtok(NULL, " ");
        }
        if(str != NULL){
            if(strstr(str2, dirName) != NULL){
                str[strlen(str)-1] = '\0';
                if(strcmp(str, "/") == 0)
                    printf("/%s\n", str2);
                else
                    printf("%s/%s\n", str, str2);
            }
        }
    }
    else{
        str = strtok(tmp, " ");
        strcpy(str2, str);
        for(int i=0;i<10;i++){
            str = strtok(NULL, " ");
        }
        if(str != NULL){
            if(strstr(str, dirName) != NULL){
                str[strlen(str)-1] = '\0';
                if(strcmp(str, "/") == 0)
                    printf("/%s\n", str2);
                else
                    printf("%s/%s\n", str, str2);
            }
        }
    }
    return 0;
}

void FindDir(DirectoryTree* dirTree, char* dirName, int o)
{
    char tmp[MAX_LENGTH];
    g_dir = fopen("Directory.txt", "r");

    while(fgets(tmp, MAX_LENGTH, g_dir) != NULL){
        ReadDir(dirTree, tmp, dirName, o);
    }
    fclose(g_dir);
}

void Instruction(DirectoryTree* dirTree, char* cmd)
{
	char* str;
	char* str1;
	char* str2;
	int val;
	if (strcmp(cmd, "") == 0 || cmd[0] == ' ') {
		return;
	}
	str = strtok(cmd, " ");  //공백을 기준으로 문자열 끊음

	if (strcmp(str, "mkdir") == 0) {
		str = strtok(NULL, " "); //str에 다음 토큰 저장
        val = mkdir_(dirTree, str);
        if (val == 0) {
			SaveDir(dirTree, g_dir_stack);
		}
	}
    else if(strcmp(str, "cp") == 0){
        str = strtok(NULL, " ");
        str1 = strtok(NULL, " ");
        val=mycp(dirTree,str,str1);
        if(val == 0){
            SaveDir(dirTree, g_dir_stack);
        }
    }
	else if (strcmp(str, "rm") == 0) {
		str = strtok(NULL, " ");
		val = rm(dirTree, str);
		if (val == 0) {
			SaveDir(dirTree, g_dir_stack);
		}
	}
	else if (strcmp(str, "cd") == 0) {
		str = strtok(NULL, " ");
		cd(dirTree, str);  
	}
	else if (strcmp(str, "pwd") == 0) {
		str = strtok(NULL, " ");
		pwd(dirTree, g_dir_stack, str);
	}
	else if (strcmp(str, "ls") == 0) {
		str = strtok(NULL, " ");
		if(str==NULL)
            ls(dirTree);
        else if(strcmp(str,"-a")==0)
            ls_a(dirTree);
        else if(strcmp(str,"-l")==0)
            ls_l(dirTree);
        else
            ls_al(dirTree);
	}
	else if (strcmp(str, "cat") == 0) {
		str = strtok(NULL, " ");
		val = cat(dirTree,str);
		if (val == 0) {
			SaveDir(dirTree, g_dir_stack);
		}
	}
	else if (strcmp(str, "chmod") == 0) {
		str = strtok(NULL, " ");
		val = chmod_(dirTree, str);
		if (val == 0) {
			SaveDir(dirTree, g_dir_stack);
		}
	}
    else if(strcmp(str, "chown") == 0){
        str = strtok(NULL, " ");
        val = chown_(dirTree, str);
        if(val == 0){
            SaveDir(dirTree, g_dir_stack);
        }
    }    
	else if (strcmp(str, "grep") == 0) {
		str = strtok(NULL, " ");
        if(str==NULL)
            return;
        str1 = strtok(NULL, " ");
        str = TrimString(str);
        if (strcmp(str, "-n") == 0)
            grep_n(str,str1);
        else
            grep(str,str1);
    }
	else if (strcmp(str, "find") == 0) {
		str = strtok(NULL, " ");
		find(dirTree, str);
	}
	else if (strcmp(str, "exit") == 0) {
		printf("로그아웃\n");
		exit(0);
	}
	else
	{
		printf("'%s': 명령을 찾을 수 없습니다\n", cmd);
	}
	return;
}

DirectoryTree* LoadDir()
{
	DirectoryTree* dirTree = (DirectoryTree*)malloc(sizeof(DirectoryTree));
	char tmp[MAX_LENGTH];

	g_dir= fopen("Directory.txt", "r");

	while (fgets(tmp, MAX_LENGTH, g_dir) != NULL)
	{
		ReadNode(dirTree, tmp);
	}

	fclose(g_dir);

	dirTree->current = dirTree->root;

	return dirTree;
}

int ReadNode(DirectoryTree* dirTree, char* tmp)
{
	TreeNode* NewNode = (TreeNode*)malloc(sizeof(TreeNode));
	TreeNode* tmpNode = NULL;
	char* str;

	NewNode->LeftChild = NULL;
	NewNode->RightSibling = NULL;
	NewNode->Parent = NULL;

	str = strtok(tmp, " ");
	strncpy(NewNode->name, str, MAX_NAME);
	str = strtok(NULL, " ");
	NewNode->type = str[0];
	str = strtok(NULL, " ");
	NewNode->mode = atoi(str);   //atoi함수 : 문자 스트링을 정수로 변환
	Mode2Permission(NewNode);
	str = strtok(NULL, " ");
	NewNode->SIZE = atoi(str);
	str = strtok(NULL, " ");
	NewNode->UID = atoi(str);
	str = strtok(NULL, " ");
	NewNode->GID = atoi(str);
	str = strtok(NULL, " ");
	NewNode->month = atoi(str);
	str = strtok(NULL, " ");
	NewNode->day = atoi(str);
	str = strtok(NULL, " ");
	NewNode->hour = atoi(str);
	str = strtok(NULL, " ");
	NewNode->minute = atoi(str);

	str = strtok(NULL, " ");
	if (str != NULL) {
		str[strlen(str) - 1] = '\0';
		MovePath(dirTree, str);
		NewNode->Parent = dirTree->current;
		if (dirTree->current->LeftChild == NULL) {
			dirTree->current->LeftChild = NewNode;
		}
		else {
			tmpNode = dirTree->current->LeftChild;
			while (tmpNode->RightSibling != NULL)
			{
				tmpNode = tmpNode->RightSibling;
			}
			tmpNode->RightSibling = NewNode;
		}
	}
	else {
		dirTree->root = NewNode;
		dirTree->current = dirTree->root;
	}
	return 0;
}

int Mode2Permission(TreeNode* dirNode)
{
    char buffer[4];
    int tmp;
    sprintf(buffer, "%d", dirNode->mode);

    for(int i=0;i<3;i++){
        tmp = buffer[i] - '0';
        for (int j = 2; j >= 0;j--){
          dirNode->permission[i * 3 - j + 2] = tmp >> j & 1;
        }
    }
    return 0;
}

void PrintPermission(TreeNode* dirNode)
{
    char rwx[4] = "rwx";

    for (int i = 0; i < 9; i++)
    {
        printf("%c", dirNode->permission[i] == 1 ? rwx[i % 3] : '-');
    }
}

void DestroyDir(TreeNode* dirNode) {
	if (dirNode->RightSibling != NULL) {
		DestroyDir(dirNode->RightSibling);  //재귀를 돌면서 오른쪽 노드 NULL로 변경
	}
	if (dirNode->LeftChild != NULL)
	{
		DestroyDir(dirNode->LeftChild);   //재귀 돌면왼쪽 노드 NULl로 변경
	}

	dirNode->LeftChild = NULL;
	dirNode->RightSibling = NULL;

	free(dirNode);   //malloc으로 동적 할당한 것 해제
}

void PrintHead(DirectoryTree* dirTree, Stack* dirStack)
{
    TreeNode* tmpNode = NULL;
    char tmp[MAX_DIR] = "";
    char tmp2[MAX_DIR] = "";
    char user;
    user = g_user_list->current == g_user_list->head ? '#' : '$';

    printf("%s@linux-desktop", g_user_list->current->name);
    DEFAULT;
    printf(":");
    tmpNode = dirTree->current;

    if(tmpNode == dirTree->root){
        strcpy(tmp, "/");
    }
    while(tmpNode->Parent != NULL){
        Push(dirStack, tmpNode->name);
        tmpNode = tmpNode->Parent;
    }
    while(IsEmpty(dirStack) == 0){
            strcat(tmp, "/");
            strcat(tmp ,Pop(dirStack));
    }

    strncpy(tmp2, tmp, strlen(g_user_list->current->dir));

    if(g_user_list->current == g_user_list->head){
        printf("%s", tmp);
    }
    else if(strcmp(g_user_list->current->dir, tmp2) != 0){
        printf("%s", tmp2);
    }
    else{
        tmpNode = dirTree->current;
        while(tmpNode->Parent != NULL){
            Push(dirStack, tmpNode->name);
            tmpNode = tmpNode->Parent;
        }
        Pop(dirStack);
        Pop(dirStack);
        printf("~");
        while(IsEmpty(dirStack) == 0){
            printf("/%s",Pop(dirStack));
        }
    }
    DEFAULT;
    printf("%c ", user);
}

TreeNode* IsExistDir(DirectoryTree* dirTree, char* dirName, char type)
{
    //variables
    TreeNode* return_node = NULL;

    return_node = dirTree->current->LeftChild;

    while (return_node != NULL) {
        if (strcmp(return_node->name, dirName) == 0 && return_node->type == type)
            break;
        return_node = return_node->RightSibling;
    }
    return return_node;
}

void *t_MakeDir(void *arg){
    TreeNode* tmpNode = NULL;
    char* str;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];
    char tmp3[MAX_DIR];
    int val;
    ThreadArg *t_arg = ((ThreadArg *)arg);
    strncpy(tmp, t_arg->cmd, MAX_DIR);
    
    if(strstr(t_arg->cmd, "/") == NULL){
        MakeDir(t_arg->dirTree, t_arg->cmd, 'd');
    }
    else{
        strncpy(tmp2, GetDir(t_arg->cmd), MAX_DIR);
        val = MovePath(t_arg->dirTree, tmp2);
        if(val != 0){
            printf("mkdir: '%s': 그런 파일이나 디렉터리가 없습니다\n", tmp2);
        }
        else{
            str = strtok(tmp, "/");
            while(str != NULL){
                strncpy(tmp3, str, MAX_NAME);
                str = strtok(NULL, "/");
            }
            MakeDir(t_arg->dirTree, tmp3 , 'd');
            t_arg->dirTree->current = tmpNode;
        }
    }
    pthread_exit(NULL);
}

int MakeDir(DirectoryTree* dirTree, char* dirName, char type)
{
    TreeNode* NewNode = (TreeNode*)malloc(sizeof(TreeNode));
    TreeNode* tmpNode = NULL;

    if(HasPermission(dirTree->current, 'w') != 0){
        printf("mkdir: '%s' 디렉터리를 만들 수 없습니다: 허가 거부\n", dirName);
        free(NewNode);
        return -1;
    }
    if(strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0){
        printf("mkdir: '%s' 디렉터리를 만들 수 없습니다\n", dirName);
        free(NewNode);
        return -1;
    }
    tmpNode = IsExistDir(dirTree, dirName, type);
    if(tmpNode != NULL && tmpNode->type == 'd'){
        printf("mkdir: '%s' 디렉터리를 만들 수 없습니다: 파일이 존재합니다\n", dirName);
        free(NewNode);
        return -1;
    }
    time(&ltime);
    today = localtime(&ltime);

    NewNode->LeftChild = NULL;
    NewNode->RightSibling = NULL;

    strncpy(NewNode->name, dirName, MAX_NAME);
    if(dirName[0] == '.'){
    NewNode->type = 'd';
        //rwx------
        NewNode->mode = 700;
        NewNode->SIZE = 4096;
    }
    else if(type == 'd'){
        NewNode->type = 'd';
        //rwxr-xr-x
        NewNode->mode = 755;
        NewNode->SIZE = 4096;
    }
    else{
        NewNode->type = 'f';
        //rw-r--r--
        NewNode->mode = 644;
        NewNode->SIZE = 0;
    }
    Mode2Permission(NewNode);
    NewNode->UID = g_user_list->current->UID;
    NewNode->GID = g_user_list->current->GID;
    NewNode->month = today->tm_mon + 1;
    NewNode->day = today->tm_mday;
    NewNode->hour = today->tm_hour;
    NewNode->minute = today->tm_min;
    NewNode->Parent = dirTree->current;

    if(dirTree->current->LeftChild == NULL){
        dirTree->current->LeftChild = NewNode;
    }
    else{
        tmpNode = dirTree->current->LeftChild;

        while(tmpNode->RightSibling!= NULL){
            tmpNode = tmpNode->RightSibling;
        }
        tmpNode->RightSibling = NewNode;
    }

    return 0;
}

//rm
int RemoveDir(DirectoryTree* dirTree, char* dirName)
{
    TreeNode* DelNode = NULL;
    TreeNode* tmpNode = NULL;
    TreeNode* prevNode = NULL;

    tmpNode = dirTree->current->LeftChild;

    if(tmpNode == NULL){
        printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", dirName);
        return -1;
    }

    if(strcmp(tmpNode->name, dirName) == 0){
        dirTree->current->LeftChild = tmpNode->RightSibling;
        DelNode = tmpNode;
        if(DelNode->LeftChild != NULL)
            DestroyDir(DelNode->LeftChild);
        free(DelNode);
    }
    else{
        while(tmpNode != NULL){
            if(strcmp(tmpNode->name, dirName) == 0){
                DelNode = tmpNode;
                break;
            }
            prevNode = tmpNode;
            tmpNode = tmpNode->RightSibling;
        }
        if(DelNode != NULL){
            prevNode->RightSibling = DelNode->RightSibling;

            if(DelNode->LeftChild != NULL)
                DestroyDir(DelNode->LeftChild);
            free(DelNode);
        }
        else{
            printf("rm: '%s'를 지울 수 없음: 그런 파일이나 디렉터리가 없습니다\n", dirName);
            return -1;
        }
    }
    return 0;
}

//cd
int MoveCurrent(DirectoryTree* dirTree, char* dirPath)
{
    TreeNode* tmp_node = NULL;
    if (strcmp(dirPath, ".") == 0) {
    }
    else if (strcmp(dirPath, "..") == 0) {
        if (dirTree->current != dirTree->root) {
            dirTree->current = dirTree->current->Parent;
        }
    }
    else {

        tmp_node = IsExistDir(dirTree, dirPath, 'd');
        if (tmp_node != NULL) {
            dirTree->current = tmp_node;
        }
        else
            return -1;
    }
    return 0;
}

int MovePath(DirectoryTree* dirTree, char* dirPath)
{
    TreeNode* tmpNode = NULL;
    char tmpPath[MAX_DIR];
    char* str = NULL;
    int val = 0;

    strncpy(tmpPath, dirPath, MAX_DIR);
    tmpNode = dirTree->current;
    //if input is root
    if (strcmp(dirPath, "/") == 0)
    {
        dirTree->current = dirTree->root;
    }
    else {
        //if input is absolute path
        if (strncmp(dirPath, "/", 1) == 0)
        {
            if (strtok(dirPath, "/") == NULL)
            {
                return -1;
            }
            dirTree->current = dirTree->root;
        }
        //if input is relative path
        str = strtok(tmpPath, "/");
        while (str != NULL) {
            val = MoveCurrent(dirTree, str);
            //if input path doesn't exist
            if (val != 0)
            {
                dirTree->current = tmpNode;
                return -1;
            }
            str = strtok(NULL, "/");
        }
    }
    return 0;
}



void PrintHelp()
{
    printf("Try 'cd --help' for more information.\n");
}

int cd(DirectoryTree* dirTree, char* cmd)
{
    TreeNode* tmpNode = NULL;
    char* str = NULL;
    char tmp[MAX_DIR];
    int val;

    if (cmd == NULL) {
        strcpy(tmp, g_user_list->current->dir);
        MovePath(dirTree, tmp);
    }
    else if (cmd[0] == '-')
    {
        if (strcmp(cmd, "--help") == 0)
        {
            printf("사용법: cd 디렉터리...\n");
            printf("  Change the shell working directory.\n\n");
            printf("  Options:\n");
            printf("  파일 수정 중 :wq 입력시 저장 후 종료\n");
            printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
            return -1;
        }
        else
        {
            str = strtok(cmd, "-");
            if (str == NULL) {
                printf("cd: 잘못된 연산자\n");
                PrintHelp();
                return -1;
            }
            else {
                printf("cd: 부적절한 옵션 -- '%s'\n", str);
                PrintHelp();
                return -1;
            }
        }
    }
    else {
        tmpNode = IsExistDir(dirTree, cmd, 'd');
        if (tmpNode != NULL)
        {
            if (HasPermission(tmpNode, 'r') != 0) {
                printf("-bash: cd: '%s': 허가거부\n", cmd);
                return -1;
            }
        }
        tmpNode = IsExistDir(dirTree, cmd, 'f');
        if (tmpNode != NULL)
        {
            printf("-bash: cd: '%s': 디렉터리가 아닙니다\n", cmd);
            return -1;
        }
        val = MovePath(dirTree, cmd);
        if (val != 0)
            printf("-bash: cd: '%s': 그런 파일이나 디렉터리가 없습니다\n", cmd);
    }
    return 0;
}


//pwd
int PrintPath(DirectoryTree* dirTree, Stack* dirStack)
{

    TreeNode* tmpNode = NULL;
    tmpNode = dirTree->current;
    //if current directory is root
    if(tmpNode == dirTree->root){
        printf("/");
    }
    else{
        //until current directory is root, repeat Push
        while(tmpNode->Parent != NULL){
            Push(dirStack, tmpNode->name);
            tmpNode = tmpNode->Parent;
        }
    //until stack is empty, repeat Pop
        while(IsEmpty(dirStack) == 0){
            printf("/");
            printf("%s",Pop(dirStack));
        }
    }
    printf("\n");
    return 0;
}

//cat
int Concatenate(DirectoryTree* dirTree, char* fName, int o)
{
    UserNode* tmpUser = NULL;
    TreeNode* tmpNode = NULL;
    FILE* fp;
    char buf[MAX_BUFFER];
    char tmpName[MAX_NAME];
    char* str;
    int tmpSIZE = 0;
    int cnt = 1;

    //file read
    if(o != 0){
        if(o == 4){
            tmpUser = g_user_list->head;
            while(tmpUser != NULL){
                printf("%s:x:%d:%d:%s:%s\n", tmpUser->name, tmpUser->UID, tmpUser->GID, tmpUser->name, tmpUser->dir);
                tmpUser = tmpUser->LinkNode;
            }
            return 0;
        }
        tmpNode = IsExistDir(dirTree,fName, 'f');

        if(tmpNode == NULL){
            return -1;
        }
        fp = fopen(fName, "r");

        while(feof(fp) == 0){
            fgets(buf, sizeof(buf), fp);
            if(feof(fp) != 0){
                break;
            }
            if(o == 2){
                if(buf[strlen(buf)-1] == '\n'){
                    printf("     %d ",cnt++);
                }
            }
            else if(o == 3){
                if(buf[strlen(buf)-1] == '\n' && buf[0] != '\n'){
                    printf("     %d ",cnt++);
                }
            }
            fputs(buf, stdout);
        }
        fclose(fp);
    }
    else{
        fp = fopen(fName, "w");
        while(fgets(buf, sizeof(buf), stdin)){
            if (strcmp(buf, ":wq\n") == 0)
                break;
            fputs(buf, fp);
            tmpSIZE += strlen(buf)-1;
        }
        fclose(fp);
        tmpNode = IsExistDir(dirTree, fName, 'f');
        if(tmpNode != NULL){
            time(&ltime);
            today = localtime(&ltime);
            tmpNode->month = today->tm_mon + 1;
            tmpNode->day = today->tm_mday;
            tmpNode->hour = today->tm_hour;
            tmpNode->minute = today->tm_min;
        }
        else{
            MakeDir(dirTree, fName, 'f');
        }
        tmpNode = IsExistDir(dirTree, fName, 'f');
        tmpNode->SIZE = tmpSIZE;
    }
    return 0;
}

// ls

void ls(DirectoryTree* dir_tree) {
    int count = 1;
    TreeNode* tmpNode = dir_tree->current;
    if (tmpNode->LeftChild == NULL)
        printf("directory empty\n");
    else {
        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            if (strlen(tmpNode->name) < 8)
                printf("%s\t\t", tmpNode->name);
            else
                printf("%s\t", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
            if (count % 5 == 0)
                printf("\n");
            count++;
        }
        printf("%s\t\n", tmpNode->name);
    }
}
void ls_a(DirectoryTree* dir_tree) {
    int count = 1;
    TreeNode* tmpNode = dir_tree->current;
    if (tmpNode->LeftChild == NULL) {
        printf(".\t\t..\n");
    }
    else {
        printf(".\t\t..\t\t");
        count = count + 2;
        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            if (strlen(tmpNode->name) < 8)
                printf("%s\t\t", tmpNode->name);
            else
                printf("%s\t", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
            if (count % 5 == 0)
                printf("\n");
            count++;
        }
        printf("%s\t\n", tmpNode->name);
    }
}
void PrintLs(TreeNode* tmpnode)//디렉토리의 정보 출력
{
    printf("%c", tmpnode->type);
    PrintPermission(tmpnode);
    printf("%6d%6d%6d", tmpnode->SIZE, tmpnode->UID, tmpnode->GID);
    printf("%5d(month)%5d(day)%5d(hour)%5d(min) ", tmpnode->month, tmpnode->day, tmpnode->hour, tmpnode->minute);
}
void ls_l(DirectoryTree* dir_tree) {
    time_t timer;
    TreeNode* tmpNode = dir_tree->current;
    timer = time(NULL);
    if (tmpNode->LeftChild == NULL)
        printf("directory empty\n");
    else {
        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            PrintLs(tmpNode);
            printf("%s\n", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
        }
        PrintLs(tmpNode);
        printf("%s\n", tmpNode->name);
    }
}

void ls_al(DirectoryTree* dir_tree) {
    time_t timer;
    TreeNode* tmpNode = dir_tree->current;
    timer = time(NULL);
    if (tmpNode->LeftChild == NULL) {
        //.
        PrintLs(tmpNode);
        printf(".\n");
        //..
        if (strcmp(dir_tree->current->name, "/") == 0) {
            PrintLs(tmpNode);
            printf("..\n");
        }
        else {
            PrintLs(tmpNode->Parent);
            printf("..\n");
        }
    }
    else {
        //.
        PrintLs(tmpNode);
        printf(".\n");
        //..
        if (strcmp(dir_tree->current->name, "/") == 0) {
            PrintLs(tmpNode);
            printf("..\n");
        }
        else {
            PrintLs(tmpNode->Parent);
            printf("..\n");
        }


        tmpNode = tmpNode->LeftChild;
        while (tmpNode->RightSibling != NULL) {
            PrintLs(tmpNode);
            printf("%s\n", tmpNode->name);
            tmpNode = tmpNode->RightSibling;
        }
        PrintLs(tmpNode);
        printf("%s\n", tmpNode->name);
    }
}

// find

int find(DirectoryTree* dirTree, char* cmd)
{
    char* str;
    if (cmd == NULL) {
        FindDir(dirTree, dirTree->current->name, 1);
        return 0;
    }
    else if (cmd[0] == '-') {
        if (strcmp(cmd, "-name") == 0) {
            str = strtok(NULL, " ");
            if (str == NULL) {
                TextError(1,NULL);
                return -1;
            }
            FindDir(dirTree, str, 0);
        }
        else if (strcmp(cmd, "--help") == 0) {
            TextError(2,NULL);
            return -1;
        }
        else {
            str = strtok(cmd, "-");
            if (str == NULL) {
                TextError(1,NULL);
                return -1;
            }
            else {
                TextError(3,str);
                return -1;
            }
        }
    }
    else {
        FindDir(dirTree, cmd, 1);
    }

    return 0;
}

//grep
char* TrimString(char* str){
    if(str[0]=='\"'){
        char ret[MAX_BUFFER];
        strncpy(ret, str + 1, strlen(str)-2);
        ret[strlen(str)-2] = '\0';
        strcpy(str, ret);
    }
    return str;
}

void grep(char* searching_word, char* f_name) {
    int i = 1;
    char output_line[MAX_LENGTH];
    FILE* fp = fopen(f_name, "rt");
    if (fp == NULL)
        printf("cannot read the file\n");
    while (1) {
        if (feof(fp))
            break;
        else
            fgets(output_line, sizeof(output_line), fp);
        i++;
    }
    FILE* fp2 = NULL;
    fp2 = fopen(f_name, "rt");
    for (int j = 1; j < i - 1; j++) {
        fgets(output_line, sizeof(output_line), fp2);
        if (strstr(output_line, searching_word) != NULL)
            printf("%s", output_line);
    }
    fclose(fp);
}
// grep 과 동일 grep_n
void grep_n(char* searching_word, char* f_name) {
    int i = 1;
    char output_line[MAX_LENGTH];
    FILE* fp = fopen(f_name, "rt");
    if (fp == NULL)
        printf("cannot read the file\n");
    while (1) {
        if (feof(fp))
            break;
        else
            fgets(output_line, sizeof(output_line), fp);
        i++;
    }
    FILE* fp2 = NULL;
    fp2 = fopen(f_name, "rt");
    for (int j = 1; j < i - 1; j++) {
        fgets(output_line, sizeof(output_line), fp2);
        if (strstr(output_line, searching_word) != NULL)
            printf("%d:%s", j, output_line);
    }
    fclose(fp);
}
void TextError(int a,char* str) {

    switch (a) {
    case 1:
        printf("find 잘못된 연산자\n");
        printf("Try 'find --help' for more information.\n");
        break;
    case 2:
        printf("사용법: find[<옵션>]... [<파일>]...\n");
        printf("\n");
        printf("  Options:\n");
        printf("    -name\t finds file by name\n");
        printf("        --help\t 이 도움말을 표시하고 끝냅니다\n");
        break;
    case 3:
        printf("find: 부적절한 옵션 -- '%s'\n", str);
        printf("Try 'find --help' for more information.\n");
        break;
    }
}

char* GetDir(char* dirPath)
{
    char* tmpPath = (char*)malloc(MAX_DIR);
    char* str = NULL;
    char tmp[MAX_DIR];
    char tmp2[MAX_DIR];

    strncpy(tmp, dirPath, MAX_DIR);
    str = strtok(dirPath, "/");
    while(str != NULL){
        strncpy(tmp2, str, MAX_DIR);
        str  = strtok(NULL, "/");
    }
    strncpy(tmpPath, tmp, strlen(tmp)-strlen(tmp2)-1);
    tmpPath[strlen(tmp)-strlen(tmp2)-1] = '\0';

    return tmpPath;
}

//mycp
int mycp(DirectoryTree* dirTree,char* sName, char* oName){

    //printf("source : %s\n",sName);
    //printf("object : %s\n\n",oName);

    char buf[1024];
    int in, out;
    int nread;

    if(access(sName,F_OK) != 0) {
        printf("원본 파일이 존재하지 않습니다.\n");
        return -1;
    }
    if(strcmp(sName,oName) == 0) {
        printf("원본과 대상이 같습니다.\n");
        return -1;
    }

    in = open(sName,O_RDONLY); //원본파일
    out = open(oName, O_WRONLY| O_CREAT, S_IRUSR| S_IWUSR);//만들파일
    nread = read(in,buf,sizeof(buf)); //읽은만큼 nread가 올라가고
    write(out,buf,nread);          //read만큼 쓴다.

    MakeDir(dirTree, oName, 'f');

   return 0;
}

// save & load
void GetPath(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack)
{
    TreeNode* tmp_node = NULL;
    char tmp[MAX_DIR] = "";

    tmp_node = dirNode->Parent;

    if (tmp_node == dirTree->root) {
        strcpy(tmp, "/");
    }
    else {
        while (tmp_node->Parent != NULL) {
            Push(dirStack, tmp_node->name);
            tmp_node = tmp_node->Parent;
        }
        while (IsEmpty(dirStack) == 0) {
            strcat(tmp, "/");
            strcat(tmp, Pop(dirStack));
        }
    }
    fprintf(g_dir, " %s\n", tmp);
}

void WriteNode(DirectoryTree* dirTree, TreeNode* dirNode, Stack* dirStack)
{
    fprintf(g_dir, "%s %c %d ", dirNode->name, dirNode->type, dirNode->mode);
    fprintf(g_dir, "%d %d %d %d %d %d %d", dirNode->SIZE, dirNode->UID, dirNode->GID, dirNode->month, dirNode->day, dirNode->hour, dirNode->minute);

    if (dirNode == dirTree->root)
        fprintf(g_dir, "\n");
    else
        GetPath(dirTree, dirNode, dirStack);

    if (dirNode->RightSibling != NULL) {
        WriteNode(dirTree, dirNode->RightSibling, dirStack);
    }
    if (dirNode->LeftChild != NULL) {
        WriteNode(dirTree, dirNode->LeftChild, dirStack);
    }
}

void SaveDir(DirectoryTree* dirTree, Stack* dirStack)
{
    g_dir = fopen("Directory.txt", "w");
    WriteNode(dirTree, dirTree->root, dirStack);
    fclose(g_dir);
}

//stack
//stack function
int IsEmpty(Stack* dir_stack)
{
    if (dir_stack->TopNode == NULL) {
        return -1;
    }
    return 0;
}

Stack* InitializeStack()
{
    Stack* returnStack = (Stack*)malloc(sizeof(Stack));

    if (returnStack == NULL) {
        printf("error occurred, returnStack.\n");
        return NULL;
    }
    //initialize Stack
    returnStack->TopNode = NULL;
    return returnStack;
}

int Push(Stack* dir_stack, char* dir_name)
{
    StackNode* dir_node = (StackNode*)malloc(sizeof(StackNode));

    if (dir_stack == NULL) {
        printf("error occurred, dir_stack.\n");
        return -1;
    }
    if (dir_node == NULL) {
        printf("error occurred, dir_node.\n");
        return -1;
    }
    //set dir_node
    strncpy(dir_node->name, dir_name, MAX_NAME);
    dir_node->LinkNode = dir_stack->TopNode;
    //set dir_stack
    dir_stack->TopNode = dir_node;
    return 0;
}
char* Pop(Stack* dir_stack)
{
    StackNode* returnNode = NULL;
    if (dir_stack == NULL) {
        printf("error occurred, dir_stack.\n");
        return NULL;
    }
    if (IsEmpty(dir_stack) == -1) {
        printf("Stack Empty.\n");
        return NULL;
    }
    returnNode = dir_stack->TopNode;
    dir_stack->TopNode = returnNode->LinkNode;

    return returnNode->name;
}

//User
void WriteUser(UserList* userList, UserNode* userNode)
{
    time(&ltime);
    today = localtime(&ltime);

    userList->current->year = today->tm_year + 1900;
    userList->current->month = today->tm_mon + 1;
    userList->current->wday = today->tm_wday;
    userList->current->day = today->tm_mday;
    userList->current->hour = today->tm_hour;
    userList->current->minute = today->tm_min;
    userList->current->sec = today->tm_sec;

    fprintf(g_user, "%s %d %d %d %d %d %d %d %d %d %s\n", userNode->name, userNode->UID, userNode->GID, userNode->year, userNode->month, userNode->wday, userNode->day, userNode->hour, userNode->minute, userNode->sec, userNode->dir);

    if (userNode->LinkNode != NULL) {
        WriteUser(userList, userNode->LinkNode);
    }

}

void SaveUserList(UserList* userList)
{
    g_user = fopen("User.txt", "w");
    WriteUser(userList, userList->head);
    fclose(g_dir);
}

int ReadUser(UserList* userList, char* tmp)
{
    UserNode* NewNode = (UserNode*)malloc(sizeof(UserNode));
    char* str;

    NewNode->LinkNode = NULL;

    str = strtok(tmp, " ");
    strncpy(NewNode->name, str, MAX_NAME);
    str = strtok(NULL, " ");
    NewNode->UID = atoi(str);
    str = strtok(NULL, " ");
    NewNode->GID = atoi(str);
    str = strtok(NULL, " ");
    NewNode->year = atoi(str);
    str = strtok(NULL, " ");
    NewNode->month = atoi(str);
    str = strtok(NULL, " ");
    NewNode->wday = atoi(str);
    str = strtok(NULL, " ");
    NewNode->day = atoi(str);
    str = strtok(NULL, " ");
    NewNode->hour = atoi(str);
    str = strtok(NULL, " ");
    NewNode->minute = atoi(str);
    str = strtok(NULL, " ");
    NewNode->sec = atoi(str);
    str = strtok(NULL, " ");
    str[strlen(str) - 1] = '\0';
    strncpy(NewNode->dir, str, MAX_DIR);

    if (strcmp(NewNode->name, "root") == 0) {
        userList->head = NewNode;
        userList->tail = NewNode;
    }
    else {
        userList->tail->LinkNode = NewNode;
        userList->tail = NewNode;
    }
    return 0;
}

UserList* LoadUserList()
{
    UserList* userList = (UserList*)malloc(sizeof(UserList));
    char tmp[MAX_LENGTH];
    g_user = fopen("User.txt", "r");

    while (fgets(tmp, MAX_LENGTH, g_user) != NULL) {
        ReadUser(userList, tmp);
    }

    fclose(g_user);
    userList->current = NULL;
    return userList;
}

UserNode* IsExistUser(UserList* userList, char* userName)
{
    UserNode* returnUser = NULL;
    returnUser = userList->head;

    while (returnUser != NULL) {
        if (strcmp(returnUser->name, userName) == 0)
            break;
        returnUser = returnUser->LinkNode;
    }
    return returnUser;
}

//permission
int CheckId(TreeNode* dirnode, char o, int i)
{
    if (o == 'r') {
        if (dirnode->permission[i] == 0)
            return -1;
        else
            return 0;
    }
    else if (o == 'w') {
        if (dirnode->permission[i + 1] == 0)
            return -1;
        else
            return 0;
    }
    else if (o == 'x') {
        if (dirnode->permission[i + 2] == 0)
            return -1;
        else
            return 0;
    }
    return -1;
}

//haspermission
int HasPermission(TreeNode* dir_node, char o)
{
    if (g_user_list->current->UID == 0)
        return 0;
    if (g_user_list->current->UID == dir_node->UID) { 
        return CheckId(dir_node, o, 0);
    }
    else if (g_user_list->current->GID == dir_node->GID) {
        return CheckId(dir_node, o, 3);
    }
    else {
        return CheckId(dir_node, o, 6);
    }
    return -1;
}

void Login(UserList* user_list, DirectoryTree* dir_tree)
{
    UserNode* tmp_user = NULL;
    char username[MAX_NAME];
    char tmp[MAX_DIR];

    tmp_user = user_list->head;

    printf("Users: ");
    while (tmp_user != NULL) {
        printf("%s ", tmp_user->name);
        tmp_user = tmp_user->LinkNode;
    }
    printf("\n");

    while (1) {
        printf("Login as: ");
        fgets(username, sizeof(username), stdin);
        username[strlen(username) - 1] = '\0';
        if (strcmp(username, "exit") == 0) {
            exit(0);
        }
        tmp_user = IsExistUser(user_list, username);
        if (tmp_user != NULL) {
            user_list->current = tmp_user;
            break;
        }
        printf("'%s' 유저가 존재하지 않습니다\n", username);
    }
    strcpy(tmp, user_list->current->dir);
    MovePath(dir_tree, tmp);
}
