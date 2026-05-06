#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <sys/wait.h>

typedef enum
{
    ROLE_INSPECTOR=0,
    ROLE_MANAGER=1
}Role;

typedef struct 
{
    Role role;
    char user[20];
    char command[20];
    char district_id[20];
    int report_id;
    int threshold_value;
    char *filter_conditions[10];
}Command_arguments_t;

typedef struct report
{
    char inspector_name[20];
    char category[20];
    char description[100];
    int id;
    time_t timestamp;
    float GPS_N, GPS_E;
    int severity_level; // integer: 1 = minor, 2 = moderate, 3 = critical
}Report;

int create_dir_and_set_perm(const char *district_id)
{
    printf("%s\n", district_id);

    char path[100];

    // make directory, have to check if it created successfully
    if(mkdir(district_id, 0750) != 0)
    {
        perror("Directory could not be created\n");
        exit(EXIT_FAILURE);
    }

    // int open(const char *pathname, int flags, mode_t mode);
    sprintf(path, "%s/reports.dat", district_id);
    int fd = 0;
    fd = open(path, O_CREAT | O_APPEND | O_RDWR,0664); // create 'reports.dat' file
    char link_path[100];
    sprintf(link_path, "active-reports-%s", district_id);
    symlink(path, link_path);
    close(fd);

    sprintf(path, "%s/district.cfg", district_id);
    fd = open(path, O_CREAT | O_RDWR, 0640);
    close(fd);

    sprintf(path, "%s/logged_district", district_id);
    fd = open(path, O_CREAT, 0644);
    close(fd);

    // char link_path[100];
    // sprintf(link_path, "active-reports-%s", district_id);
    // symlink(path, link_path);

    return 0;
}
int add(char *district_id, Report rep)
{
    // parse each district directory for its id and
    // check if it's the right one

    DIR *current_dir = NULL;
    current_dir = opendir("."); // open containing directory

    struct dirent *directory_element = NULL;

    int dist_exists = 0;
    while((directory_element = readdir(current_dir)) != NULL)
    {
        
        // for each element we must check if it is a folder (different from . and ..) (and not a symbolic link)

        struct stat file;
        lstat(directory_element->d_name, &file); // extract info about each file in the containing directory

        // check if 'element' is a folder
        if(S_ISDIR(file.st_mode)==1 && (strcmp("..", directory_element->d_name)!=0 && strcmp(".", directory_element->d_name)!=0))
        {
            // district directory exists
            if(strcmp(district_id, directory_element->d_name) == 0)
            {
                // printf("foundya folder\n");
                dist_exists = 1;
                // open files and write to them...
                char path[100];
                sprintf(path, "%s/reports.dat", district_id);
                int fd_reports = open(path, O_RDWR | O_APPEND | O_CREAT, 0664);
                write(fd_reports, &rep, sizeof(Report));
                fchmod(fd_reports, 0664);
                close(fd_reports);

                return 0;
            }
        }
    }
    // directory does not exist, we have to create it and its files
    if(dist_exists == 0)
    {
        create_dir_and_set_perm(district_id);
        char path[100];
        sprintf(path,"%s/reports.dat", district_id);
        int fd = open(path, O_WRONLY | O_APPEND);
        if(fd == -1)
        {
            return -1;
        }
        if( write(fd, &rep, sizeof(Report))!=sizeof(Report) )
        {
            return -1;
        }
        close(fd);
    }

    // close the containing directory
    if(current_dir != NULL)
        closedir(current_dir);
    return 0;
}
void convert_permissions(char *result, const char *FILE_PATH)
{
    char str[10];
    int i = 0;
    struct stat filedet;
    lstat(FILE_PATH, &filedet);
    if(filedet.st_mode & S_IRUSR) str[i] = 'r'; else str[i] = '-'; i++;
    if(filedet.st_mode & S_IWUSR) str[i] = 'w'; else str[i] = '-'; i++;
    if(filedet.st_mode & S_IXUSR) str[i] = 'x'; else str[i] = '-'; i++;

    if(filedet.st_mode & S_IRGRP) str[i] = 'r'; else str[i] = '-'; i++;
    if(filedet.st_mode & S_IWGRP) str[i] = 'w'; else str[i] = '-'; i++;
    if(filedet.st_mode & S_IXGRP) str[i] = 'x'; else str[i] = '-'; i++;

    if(filedet.st_mode & S_IROTH) str[i] = 'r'; else str[i] = '-'; i++;
    if(filedet.st_mode & S_IWOTH) str[i] = 'w'; else str[i] = '-'; i++;
    if(filedet.st_mode & S_IXOTH) str[i] = 'x'; else str[i] = '-'; i++;
    
    str[i] = '\0';
    
    strcpy(result, str);
    return;
}
void list(const char *district_id)
{
    char path[100];
    sprintf(path, "%s/reports.dat", district_id);
    struct stat file_det;
    lstat(path, &file_det);
    // print file permissions
    char str[10];
    convert_permissions(str, path);
    printf("%s\n", str);
    // file size and last modification time
    // file_det.st_size field (off_t type -> signed int on linux -> 4 bytes)
    // file_det.st_mtime field (time_t type -> no. of seconds -> so i need to work it in HH:MM:SS)
    printf("Size of %s/reports.dat: %d bytes\n", district_id, (int)file_det.st_size);
    struct tm *time_data = localtime(&file_det.st_mtime);
    char time[20];
    // use strftime(char *str, size_t maxsize, const char *format, const struct tm *timeptr) function
    strftime(time, sizeof(time), "%x %X", time_data);
    printf("Time of last modify: %s\n", time);

    // open file for reading
    int fd = open(path, O_RDWR);
    if(fd == -1)
    {
        printf("File %s does not exist\n", path);
        exit(-1);
    }
    //parse file and read sizeof(Report) bytes
    Report r;
    while( (sizeof(Report)) == (read(fd, &r, sizeof(Report))) )
    {
        time_data = localtime(&r.timestamp);
        strftime(time, sizeof(time), "%x %X", time_data);
        printf("Report ID: %d\nInspector Name: %s\nGPS_N: %f, GPS_E: %f\nCategory: %s\nSeverity: %d\nDate: %s\nDescription: %s\n",
            r.id, r.inspector_name, r.GPS_N, r.GPS_E, r.category, r.severity_level, time, r.description);
    }

    close(fd);
    return;
}
void view(const char *district_id, int report_id)
{
    // id e generat random, acum trebuie sa il caut manual, trecut prin tot
    char path[100];
    sprintf(path, "%s/reports.dat", district_id);
    int fd = open(path, O_RDWR);
    if(fd == -1)
    {
        printf("File %s does not exist\n", path);
        exit(-1);
    }
    Report r;
    int found = 0;
    while( (sizeof(Report)) == (read(fd, &r, sizeof(Report))) && (found==0))
    {
        if(r.id == report_id)
        {
            found = 1;
            struct tm *time_data = localtime(&r.timestamp);
            char time[20];
            strftime(time, sizeof(time), "%x %X", time_data);
            printf("Report ID: %d\nInspector Name: %s\nGPS_N: %f, GPS_E: %f\nCategory: %s\nSeverity: %d\nDate: %s\nDescription: %s\n",
            r.id, r.inspector_name, r.GPS_N, r.GPS_E, r.category, r.severity_level, time, r.description);
        }
    }
    if(found==0)
    {
        printf("Report with id '%d' could not be found!\n", report_id);
        exit(-1);
    }
    return;
}
void remove_report(const char *district_id, int report_id)
{
    char path[100];
    sprintf(path, "%s/reports.dat", district_id);
    int fd = open(path, O_RDWR);
    if (fd==-1)
    {
        printf("File %s does not exist\n", path);
        exit(-1);
    }

    Report r;
    int record_count = 0;
    int delete_position = -1;

    // looking for the record
    while (read(fd, &r, sizeof(Report)) == sizeof(Report))
    {
        if (r.id == report_id)
        {
            delete_position = record_count*sizeof(Report);
        }
        record_count++;
    }

    if (record_count == 0)
    {
        printf("Reports file has no records!\n");
        close(fd);
        exit(-1);
    }

    if (delete_position == -1)
    {
        printf("Report with id '%d' could not be found!\n", report_id);
        close(fd);
        exit(-1);
    }

    int last_position = (record_count-1)*sizeof(Report);

    // last record
    if (delete_position == last_position)
    {
        if (ftruncate(fd, delete_position) != 0)
        {
            printf("There was a problem truncating the file!\n");
            close(fd);
            exit(-1);
        }
        close(fd);
        return;
    }
    // record is somewhere else
    int read_position = delete_position + sizeof(Report);
    int write_position = delete_position;
    while (read_position < record_count*sizeof(Report))
    {
        lseek(fd, read_position, SEEK_SET);
        if (read(fd, &r, sizeof(Report)) != sizeof(Report))
        {
            close(fd);
            exit(-1);
        }
        lseek(fd, write_position, SEEK_SET);
        if (write(fd, &r, sizeof(Report)) != sizeof(Report))
        {
            printf("There was a problem deleting the record!\n");
            close(fd);
            exit(-1);
        }
        read_position += sizeof(Report);
        write_position += sizeof(Report);
    }
    if (ftruncate(fd, last_position) != 0)
    {
        printf("There was a problem truncating the file!\n");
        close(fd);
        exit(-1);
    }
    close(fd);
}
void update_threshold(const char *district_id, int severity_value)
{
    /*update_threshold <district_id> <value> 
    Update the severity threshold in district.cfg.
    Manager role only. Before writing, call lstat() on district.cfg and
    verify the permission bits match 640; if someone has changed them, 
    refuse and print a diagnostic.*/
    
    DIR *current_dir = NULL;
    current_dir = opendir(".");

    struct dirent *directory_element = NULL;
    // parse the directory to modify the threshold value
    // in the district.cfg file of the specified district
    int dest_exists = 0;
    while((directory_element = readdir(current_dir)) != NULL)
    {
        // found the right district directory
        if(strcmp(directory_element->d_name, district_id)==0)
        {
            dest_exists = 1;
            char path[100];
            sprintf(path, "%s/district.cfg", district_id);
            struct stat file_det;
            lstat(path, &file_det);
            // checking file permission bits
            if((file_det.st_mode & 0777) != 0640)
            {
                printf("Incorrect permissions for district.cfg in %s", district_id);
                exit(-1);
            }

            int fd = open(path, O_RDWR);
            if(fd != -1) 
            {
                char line[50];
                sprintf(line, "severity_threshold = %d", severity_value);
                write(fd, line, sizeof("severity_threshold = 3")-1);
                close(fd);
            }
        }
    }
    if(dest_exists == 0)
    {
        // means that there's no such district logged
        // and we cannot write to said non-existent district
        printf("Specified district does not exist! '%s'\n", district_id);
        exit(-1);
    }

    if(current_dir)
        closedir(current_dir);
    return;
}
// deletes entire district folder and its symlink (if existing)
int remove_district(const char *district_id)
{
    // can use unlink() to delete things
    char path[100];
    memset(path, 0, sizeof(path));
    sprintf(path, "active-reports-%s", district_id);
    if(unlink(path) == -1)
    {
        printf("could not delete the symlink!\n");
    }
    sprintf(path, "%s",  district_id);
    pid_t kidpid = fork();
    if(kidpid == -1)
    {
        return -1;
    }
    else if(kidpid == 0)
    {
        // execlp("ls", "ls", path, NULL); // safe option for now
        execlp("rm", "rm", "-rf", path, NULL);
    }
    else
    {
        wait(NULL);
    }
    return 0;
}
// still need to make filter function
void filter()
{
    return;
}

int parse_arguments(int argc, char *argv[], Command_arguments_t *cArgs)
{
    if(argc==2 && strcmp(argv[1], "--help")==0)
    {
        return 0;
    }
    if(strcmp(argv[1], "--role")!=0)
    {
        // printf("here1\n");
        return -1;
    }
    else
    {
        if(strstr(argv[2], "--")!=NULL)
        {
            // third argument is not valid
            // printf("here2\n");
            return -1;
        }
        if(strcmp(argv[2], "inspector")==0) 
        {
            cArgs->role = ROLE_INSPECTOR;
        }
        if(strcmp(argv[2], "manager")==0) 
        {
            cArgs->role = ROLE_MANAGER;
        }
    }
    if(strcmp(argv[3], "--user")!=0)
    {   
        // printf("here3\n");
        return -1;
    }
    else
    {
        if(strstr(argv[4], "--")!=NULL)
        {
            // fifth argument is not valid
            // printf("here4\n");
            return -1;
        }
        strcpy(cArgs->user, argv[4]);
    }
    if(strstr(argv[5], "--")==NULL)
    {
        // printf("here5\n");
        return -1;
    }
    else
    {
        if(strcmp(argv[5], "--add")==0) strcpy(cArgs->command,"--add");
        else if(strcmp(argv[5], "--help")==0) strcpy(cArgs->command,"--help");
        else if(strcmp(argv[5], "--list")==0) strcpy(cArgs->command,"--list");
        else if(strcmp(argv[5], "--view")==0) strcpy(cArgs->command,"--view");
        else if(strcmp(argv[5], "--remove_report")==0) strcpy(cArgs->command,"--remove_report");
        else if(strcmp(argv[5], "--update_threshold")==0) strcpy(cArgs->command,"--update_threshold");
        else if(strcmp(argv[5], "--filter")==0) strcpy(cArgs->command,"--filter");
        else if(strcmp(argv[5], "--remove_district")==0) strcpy(cArgs->command,"--remove_district");
        else 
        {
            // printf("here6\n");
            return -1;
        }
    } 
    // printf("hah, you've got to this point!\n");
    return 0;
}

void help()
{
    // print instructions on program syntax
    printf("Usage: city_manager --role <inspector|manager> --user <username> --<command> [args]\n");
    printf("Commands:\n");
    printf("  --add <district_id>\n");
    printf("  --list <district_id>\n");
    printf("  --view <district_id> <report_id>\n");
    printf("  --remove_report <district_id> <report_id>\n");
    printf("  --update_threshold <district_id> <value>\n");
    printf("  --filter <district_id> <condition1> [condition2 ...]\n");
    printf("  --remove_district <district_id>\n");
    printf("  --help\n");
    return;
}
int log_operation(const char *district_id,const char *operation, Role role, const char *user)
{
    // if the inspector is unable to write then
    // only remove_report operation and update_threshold
    // are going to be logged

    // operation, timestamp, role, user
    char path[100], the_role[10];
    if(role == ROLE_INSPECTOR) strcpy(the_role, "inspector");
    if(role == ROLE_MANAGER) strcpy(the_role, "manager");
    sprintf(path, "%s/logged_district", district_id);
    int fd = open(path, O_APPEND | O_WRONLY);
    if(fd == -1) 
    {
        return -1;
    }

    char line[100];
    char time_str[20];
    time_t now={0};
    struct tm *current_time = localtime(&now);
    strftime(time_str, sizeof(time_str), "%x %X",current_time);
    sprintf(line, "%s, %s, %s, %s\n", operation, time_str, the_role, user); 
    int bytes_written = write(fd, line, strlen(line));
    if(bytes_written != strlen(line))
    {
        return -1;
    }
    close(fd);
    return 0;
}
// modify checking the permissions for each role !!!
// must also add the check for remove_district command
int check_operation_permission(const char *filepath, Role role, const char *operation, const char *district_id)
{
    // for now
    return 0;
    struct stat file_det;
    // see if the file exists
    if(lstat(filepath, &file_det)==-1)
    {
        return -1;
    }

    mode_t file_bits = (file_det.st_mode & 0777);
    mode_t expected_bits = 0000;
    if(strstr(filepath, "reports.dat")!=NULL)
    {
        expected_bits = 0664;  // rw-rw-r--
    }
    else if(strstr(filepath, "district.cfg")!=NULL)
    {
        expected_bits = 0640;  // rw-r-----
    }
    else if(strstr(filepath, "logged_district")!=NULL)
    {
        expected_bits = 0644;  // rw-r--r--
    }
    // check file permissions
    if(file_bits != expected_bits)
    {
        return -1;
    }
    // role check
    if(role == ROLE_INSPECTOR)
    {
        // inspectors not allowed
        if(strcmp(operation, "--remove_report")==0 || strcmp(operation, "--update_threshold")==0)
        {
            return -1;
        }
        
        // list, view, filter operation
        if(strcmp(operation, "--list")==0 || strcmp(operation, "--view")==0 || strcmp(operation, "--filter")==0)
        {
            if(!(file_bits & S_IRGRP))
            {
                return -1;
            }
        }
        // add operation
        if(strcmp(operation, "--add")==0)
        {
            if(!(file_bits & S_IWGRP))
            {
                 return -1;
            }
        }
    }
    else if(role == ROLE_MANAGER)
    {        
        // add operation
        if(strcmp(operation, "--add")==0 || strcmp(operation, "--remove_report")==0 || strcmp(operation, "--update_threshold")==0)
        {
            if(!(file_bits & S_IWUSR))
            {
                return -1;
            }
        }
        // list, view, add operation
        if(strcmp(operation, "--list")==0 || strcmp(operation, "--view")==0 || strcmp(operation, "--filter")==0)
        {
            if(!(file_bits & S_IRUSR))
            {
                return -1;
            }
        }
    }
    return 0;
}
int main(int argc, char *argv[])
{
    // minimum number of arguments
    if(argc==2 && strcmp(argv[1], "--help")==0)
    {
        help();
    }
    else if(argc < 6)
    {
        printf("Incorect parameter structure!\n");
        help();
        exit(-1);
    }
    Command_arguments_t commandArgs;
    if(parse_arguments(argc, argv, &commandArgs) != 0)
    {
        printf("Error running the program!\nCheck calling arguments!\n");
        help();
        exit(-1);
    }
    
    char file_path[100];
    if( strcmp(commandArgs.command,"--add")==0 || strcmp(commandArgs.command,"--list")==0
    || strcmp(commandArgs.command,"--view")==0 || strcmp(commandArgs.command,"--remove_report")==0
    || strcmp(commandArgs.command,"--remove_report")==0 || strcmp(commandArgs.command,"--update_threshold")==0
    || strcmp(commandArgs.command, "--remove_district")==0 )
    {
        strcpy(commandArgs.district_id, argv[6]);
        // building file path
        if(strcmp(commandArgs.command, "--update_threshold")!=0)
            sprintf(file_path, "%s/reports.dat", commandArgs.district_id);
        else
            sprintf(file_path, "%s/district.cfg", commandArgs.district_id);
    }
    
    // need to check the permission for the operation and role
    if(check_operation_permission(file_path, commandArgs.role, commandArgs.command, commandArgs.district_id) != 0)
    {
        printf("Permissions for operation %s not met!\n", commandArgs.command);
        exit(-1);
    }
    // perform the operation, reading extra details will be done in main
    char line[100];
    if(strcmp(commandArgs.command, "--add")==0)
    {
        Report r;
        strcpy(r.inspector_name, commandArgs.user);
        printf("Report ID: ");fgets(line, sizeof(line), stdin);sscanf(line,"%d\n", &r.id);
        printf("Category: "); fgets(line, sizeof(line), stdin); sscanf(line, "%[^\n]\n", r.category);
        printf("GPS_N: ");fgets(line, sizeof(line), stdin);sscanf(line,"%f\n", &r.GPS_N);
        printf("GPS_E: "); fgets(line, sizeof(line), stdin); sscanf(line, "%f\n", &r.GPS_E);
        printf("Severity: "); fgets(line, sizeof(line), stdin); sscanf(line, "%d\n", &r.severity_level);
        //timestamp calculation
        struct tm dt = {0};
        printf("Date DD/MM/YYYY :");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%d/%d/%d\n", &dt.tm_mday, &dt.tm_mon, &dt.tm_year);
        printf("Time HH:MM :");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%d:%d\n", &dt.tm_hour, &dt.tm_min);
        r.timestamp = mktime(&dt);
        printf("Write a small description(single-line): ");
        fgets(line, sizeof(line)-1, stdin);
        sscanf(line, "%[^\n]\n", r.description);
                
        // trying to add the report, if it does not add then r.id must be invalid 
        if(add(commandArgs.district_id, r) == -1)
        {
            printf("Unable to add!\n");
            exit(-1);
        }
    }
    if(strcmp(commandArgs.command, "--list")==0)
    {
        list(commandArgs.district_id);
    }
    if(strcmp(commandArgs.command, "--view")==0)
    {
        int id;
        printf("ID of wanted report: ");
        fgets(line, sizeof(line)-1, stdin);
        sscanf(line, "%d", &id);
        view(commandArgs.district_id, id);
    }
    if(strcmp(commandArgs.command, "--help")==0)
    {
        help();
    }
    // these operations need to be logged by log_operation()
    if(strcmp(commandArgs.command, "--remove_report")==0)
    {
        int r_id=0;;
        printf("ID of wanted report: ");
        fgets(line, sizeof(line)-1, stdin);
        sscanf(line, "%d\n", &r_id);
        remove_report(commandArgs.district_id, r_id);
        log_operation(commandArgs.district_id, commandArgs.command, commandArgs.role, commandArgs.user);
    }
    if(strcmp(commandArgs.command, "--update_threshold")==0)
    {
        int new_thres=0;
        printf("New threshold :");
        fgets(line, sizeof(line)-1, stdin);
        sscanf(line,"%d\n", &new_thres);
        log_operation(commandArgs.district_id ,commandArgs.command, commandArgs.role, commandArgs.user);
    }
    if(strcmp(commandArgs.command, "--remove_district")==0)
    {
        // printf("%s", commandArgs.district_id);
        remove_district(commandArgs.district_id);
    }
    return 0;
}