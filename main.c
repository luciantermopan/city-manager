#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

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
    char str[10] = {0};
    int i = 0;
    str[i] = '.'; // to do later !!!!
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

    strcpy(result, str);
    return;
}

void list()
{
    /* MACROS:
    S_IRUSR: Read permission (owner)
    S_IWUSR: Write permission (owner)
    S_IXUSR: Execute permission (owner)
    (Similar constants exist for Group GRP and Others OTH) 
    */
    return;
}

void view()
{
    return;
}

void remove_report()
{
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

int main(int argc, char *argv[])
{
    // create a fake report for now..
    Report r1 = {0};
    strcpy(r1.inspector_name, "tester");
    strcpy(r1.category, "road");
    strcpy(r1.description, "pothole");
    r1.id = rand() % 1000;
    r1.timestamp = time(NULL);
    r1.GPS_N = 44.4268;
    r1.GPS_E = 26.1025;
    r1.severity_level = 2;
    
    // add(argv[1], r1);

    // update_threshold("micro15", 3);

    char str[11];
    convert_permissions(str, "micro15/reports.dat");
    str[10] = '\0';
    printf("%s", str);
    return 0;
}
