#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

typedef enum
{
    ROLE_INSPECTOR=0,
    ROLE_MANAGER=1
}Role;

typedef struct 
{
    Role role;
    char user[20];
    char command[10];
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

// have to add the symlink() behavoiur in this function !!!
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
    close(fd);

    sprintf(path, "%s/district.cfg", district_id);
    fd = open(path, O_CREAT | O_RDWR, 0640);
    close(fd);

    sprintf(path, "%s/logged_district", district_id);
    fd = open(path, O_CREAT, 0644);
    close(fd);

    return 0;
}

void add(char *district_id, Report rep)
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
                dist_exists = 1;
                // open files and write to them...
                char path[100];
                sprintf(path, "%s/reports.dat", district_id);
                int fd_reports = open(path, O_RDWR | O_APPEND);
                write(fd_reports, &rep, sizeof(Report));
                close(fd_reports);
            }

        }
    }
    // directory does not exist, we have to create it and its files
    if(dist_exists == 0)
    {
        create_dir_and_set_perm(district_id);
    }

    // close the containing directory
    if(current_dir != NULL)
        closedir(current_dir);
    return;
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
    int records_counter = 0;
    int delete_position = -1; // assume the record does not exist

    char path[100];
    sprintf(path, "%s/reports.dat", district_id);
    int fd = open(path, O_RDWR);
    if(fd == -1)
    {
        printf("File %s does not exist\n", path);
        exit(-1);
    }
    
    Report r;
    while( (sizeof(Report)) == (read(fd, &r, sizeof(Report))))
    {
        if(report_id == r.id)
        {
            delete_position = records_counter * sizeof(Report);
        }
        records_counter++;
    }
    if(records_counter == 0)
    {
        printf("Reports file has no records!\n");
        exit(-1);
    }
    if(delete_position == -1)
    {
        printf("Report with id '%d' could not be found!\n", report_id);
        exit(-1);
    }
    
    // last record 
    if(delete_position == (records_counter-1)*sizeof(Report))
    {
        if(ftruncate(fd, delete_position) != 0)
        {
            printf("There was a problem truncating the file!\n");
            if(fd)
                close(fd);
            exit(-1);
        }
    }
    // somewhere before
    else
    {
        int size = (records_counter-1)*sizeof(Report);
        int writing_pos = delete_position;
        int reading_pos = writing_pos + sizeof(Report);
        // moved the file cursor to the position of deletion
        // lseek(fd, writing_pos, SEEK_SET);
        lseek(fd, reading_pos, SEEK_SET);
        while(reading_pos <= size)
        {
            if((sizeof(Report)) == (read(fd, &r, sizeof(Report))))
            {
                lseek(fd, writing_pos, SEEK_SET);
                if( (write(fd, &r, sizeof(Report))) != sizeof(Report))
                {
                    printf("There was a problem deleting the record!\n");
                    exit(-1);
                }
                writing_pos += sizeof(Report);
                reading_pos += sizeof(Report);
                lseek(fd, reading_pos, SEEK_SET);
            }
        }
        if(ftruncate(fd, size) != 0)
        {
            printf("There was a problem truncating the file\n");
            if(fd) 
                close(fd);
            exit(-1);
        }
    }
    if(fd) 
        close(fd);
    return;
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
void filter()
{
    return;
}

int parse_arguments(int argc, char *argv[], Command_arguments_t *cArgs)
{
    if(strcmp(argv[1], "--help")==0)
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
        if(strcmp(argv[5], "--add")) strcpy(cArgs->command,"--add");
        else if(strcmp(argv[5], "--help")) strcpy(cArgs->command,"--help");
        else if(strcmp(argv[5], "--list")) strcpy(cArgs->command,"--list");
        else if(strcmp(argv[5], "--view")) strcpy(cArgs->command,"--view");
        else if(strcmp(argv[5], "--remove_report")) strcpy(cArgs->command,"--remove_report");
        else if(strcmp(argv[5], "--update_threshold")) strcpy(cArgs->command,"--update_threshold");
        else if(strcmp(argv[5], "--filter")) strcpy(cArgs->command,"--filter");
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
    printf("  --help\n");
    return;
}

int check_operation_permission(const char *filepath, Role role, const char *operation)
{
    
    return 0;
}

int main(int argc, char *argv[])
{
    // minimum number of arguments
    if(argc < 6)
    {
        printf("Incorect parameter structure!\n");
        help();
        exit(-1);
    }
    Command_arguments_t commandArgs;
    if(parse_arguments(argc, argv, &commandArgs) != 0)
    {
        printf("Error running the program!\nCheck calling arguments!\n");
    }
    Report r;
    if(strcmp(commandArgs.command,"--add")==0 || strcmp(commandArgs.command,"--list")==0
    || strcmp(commandArgs.command,"--view")==0 || strcmp(commandArgs.command,"--remove_report")==0
    || strcmp(commandArgs.command,"--remove_report")==0 || strcmp(commandArgs.command,"--update_threshold")==0)
    {
        strcpy(commandArgs.district_id, argv[6]);
    }
    
    return 0;
}
