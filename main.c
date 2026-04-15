#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

void add(char *district_id)
{
    // parse each district directory for its id and
    // check if it's the right one

    DIR *current_dir = NULL;
    current_dir = opendir("."); // open containing directory

    struct dirent *directory_element = NULL;

    while((directory_element = readdir(current_dir)) != NULL)
    {
        // printf("%s\n", directory_element->d_name);
        // for each element we must check if it is a folder (different from . and ..) (and not a symbolic link)

        struct stat file;
        lstat(directory_element->d_name, &file);
        // check if element is a folder
        if(S_ISDIR(file.st_mode)==1 && (strcmp("..", directory_element->d_name)!=0 && strcmp(".", directory_element->d_name)!=0))
        {
            if(strcmp(district_id, directory_element->d_name)!=0) // district directory does not exist
            {
                // needs to be created
                printf("%d\n",   mkdir(district_id, 0750));
                chdir(district_id);
                open("reports.dat",O_RDWR, O_CREAT | O_APPEND);
                open("district.cfg", O_RDWR, O_CREAT);
                open("logged_district", O_RDWR, O_CREAT);
                chdir("..");
            }
            else // district directory exists, now only need to add the report under reports.dat
            {

            }
        }
    }



    // close the containing directory
    if(current_dir!= NULL)
        closedir(current_dir);
    return;
}

int main(int argc, char *argv[])
{
    add(argv[1]);
    return 0;
}
