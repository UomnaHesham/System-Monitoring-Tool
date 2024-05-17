#include <ctype.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

typedef struct {
    GtkWidget *cpu_label;
    GtkWidget *mem_label;
    GtkWidget *disk_label;
    GtkWidget *cpu_chart;
    GtkWidget *mem_chart;
    GtkWidget *disk_chart;
    double cpu_usage_data[100];
    double mem_usage_data[100];
    double disk_usage_data[100];
} MonitorData;

double get_cpu_usage() {
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Error opening /proc/stat");
        return -1;
    }

    unsigned long long user, nice, system, idle;
    if (fscanf(fp, "cpu %llu %llu %llu %llu", &user, &nice, &system, &idle) != 4) {
        fclose(fp);
        perror("Error reading CPU stats");
        return -1;
    }

    fclose(fp);
    return (double)(user + nice + system) / (user + nice + system + idle);
}

void get_memory_usage(double* total_mem, double* used_mem, double* free_mem) {
    struct sysinfo mem_info;
    if (sysinfo(&mem_info) != 0) {
        perror("Error getting memory info");
        return;
    }

    *total_mem = (double)mem_info.totalram * mem_info.mem_unit;
    *free_mem = (double)mem_info.freeram * mem_info.mem_unit;
    *used_mem = *total_mem - *free_mem;
}
void get_disk_usage(double* total_disk, double* used_disk, double* free_disk) {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        perror("Error getting disk info");
        return;
    }

    *total_disk = (double)stat.f_blocks * stat.f_frsize;
    *free_disk = (double)stat.f_bfree * stat.f_frsize;
    *used_disk = *total_disk - *free_disk;
}

gboolean draw_cpu_chart(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    MonitorData *data = (MonitorData *)user_data;

    GtkAllocation allocation;
    gtk_widget_get_allocation(data->cpu_chart, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, 0, height);
    for (int i = 0; i < 100; ++i) {
        cairo_line_to(cr, i * (width / 100), height - (data->cpu_usage_data[i] * height));
    }
    cairo_line_to(cr, width, height);
    cairo_close_path(cr);

    cairo_set_source_rgb(cr, 0.4, 0.6, 0.8);
    cairo_fill(cr);

    return FALSE;
}

gboolean draw_memory_chart(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    MonitorData *data = (MonitorData *)user_data;

    GtkAllocation allocation;
    gtk_widget_get_allocation(data->mem_chart, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, 0, height);
    for (int i = 0; i < 100; ++i) {
        cairo_line_to(cr, i * (width / 100), height - (data->mem_usage_data[i] * height));
    }
    cairo_line_to(cr, width, height);
    cairo_close_path(cr);

    cairo_set_source_rgb(cr, 0.4, 0.8, 0.4);
    cairo_fill(cr);

    return FALSE;
}

gboolean draw_disk_chart(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    MonitorData *data = (MonitorData *)user_data;

    GtkAllocation allocation;
    gtk_widget_get_allocation(data->disk_chart, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, 0, height);
    for (int i = 0; i < 100; ++i) {
        cairo_line_to(cr, i * (width / 100), height - (data->disk_usage_data[i] * height));
    }
    cairo_line_to(cr, width, height);
    cairo_close_path(cr);

    cairo_set_source_rgb(cr, 0.8, 0.4, 0.4);
    cairo_fill(cr);

    return FALSE;
}

void initialize_gui(MonitorData *data) {

    gtk_init(NULL, NULL);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "System Monitor");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    data->cpu_label = gtk_label_new("");
    data->mem_label = gtk_label_new("");
    data->disk_label = gtk_label_new("");


    data->cpu_chart = gtk_drawing_area_new();
    data->mem_chart = gtk_drawing_area_new();
    data->disk_chart = gtk_drawing_area_new();


    gtk_widget_set_size_request(data->cpu_chart, 200, 100);
    gtk_widget_set_size_request(data->mem_chart, 200, 100);
    gtk_widget_set_size_request(data->disk_chart, 200, 100);


    g_signal_connect(data->cpu_chart, "draw", G_CALLBACK(draw_cpu_chart), data);
    g_signal_connect(data->mem_chart, "draw", G_CALLBACK(draw_memory_chart), data);
    g_signal_connect(data->disk_chart, "draw", G_CALLBACK(draw_disk_chart), data);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    gtk_box_pack_start(GTK_BOX(vbox), data->cpu_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data->cpu_chart, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data->mem_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data->mem_chart, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data->disk_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), data->disk_chart, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}
typedef struct {
    int pid;
    double cpu_usage;
    char name[256];
} ProcessInfo;

int get_total_processes() {
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    int total_processes = 0;

    if (dir == NULL) {
        perror("Error opening /proc directory");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            total_processes++;
        }
    }

    closedir(dir);
    return total_processes;
}
double get_process_cpu_usage(int pid) {
    char path[50];
    FILE *fp;
    unsigned long utime, stime, cutime, cstime, starttime;
    double total_time, seconds;
    double cpu_usage;
    sprintf(path, "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("Error opening process stat file");
        return -1;
    }
    if (fscanf(fp, "%*d (%*[^)]) %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %lu %lu %lu %lu %*d %*d %*d %*d %*d %*d %*d %lu",
               &utime, &stime, &cutime, &cstime, &starttime) != 5) {
        fclose(fp);
        perror("Error reading process stat file");
        return -1;
    }
    fclose(fp);
    total_time = utime + stime + cutime + cstime;
    FILE *uptime_fp = fopen("/proc/uptime", "r");
    if (uptime_fp == NULL) {
        perror("Error opening /proc/uptime");
        return -1;
    }
    double uptime;
    if (fscanf(uptime_fp, "%lf", &uptime) != 1) {
        fclose(uptime_fp);
        perror("Error reading uptime");
        return -1;
    }
    fclose(uptime_fp);
    seconds = uptime - (starttime / sysconf(_SC_CLK_TCK));
    cpu_usage = ((total_time / sysconf(_SC_CLK_TCK)) / seconds) * 100.0;

    return cpu_usage;
}

int compare_process_info(const void *a, const void *b) {
    const ProcessInfo *p1 = (const ProcessInfo *)a;
    const ProcessInfo *p2 = (const ProcessInfo *)b;
 
    if (p1->cpu_usage > p2->cpu_usage) {
        return -1;
    } else if (p1->cpu_usage < p2->cpu_usage) {
        return 1;
    } else {
        return 0;
    }
}

gboolean update_data(gpointer user_data) {
    MonitorData *data = (MonitorData *)user_data;

    static double cpu_usage = 0.5;
    double cpu_change = (rand() % 21 - 10) / 100.0; 
    cpu_usage += cpu_change;
    if (cpu_usage < 0) cpu_usage = 0;
    if (cpu_usage > 1) cpu_usage = 1;

    char cpu_text[50];
    snprintf(cpu_text, sizeof(cpu_text), "CPU Usage: %.2f%%", cpu_usage * 100);
    gtk_label_set_text(GTK_LABEL(data->cpu_label), cpu_text);

    for (int i = 0; i < 99; ++i) {
        data->cpu_usage_data[i] = data->cpu_usage_data[i + 1];
    }
    data->cpu_usage_data[99] = cpu_usage;
       
    static double mem_usage = 0.5;
    double mem_change = (rand() % 21 - 10) / 100.0; 
    mem_usage += mem_change;
    if (mem_usage < 0) mem_usage = 0;
    if (mem_usage > 1) mem_usage = 1;

    char mem_text[50];
    snprintf(mem_text, sizeof(mem_text), "Memory: %.2f MB Used / %.2f MB Free", mem_usage * 1024, (1 - mem_usage) * 1024);
    gtk_label_set_text(GTK_LABEL(data->mem_label), mem_text);


    for (int i = 0; i < 99; ++i) {
        data->mem_usage_data[i] = data->mem_usage_data[i + 1];
    }
    data->mem_usage_data[99] = mem_usage;
    static double disk_usage = 0.5;
    double disk_change = (rand() % 21 - 10) / 100.0; 
    disk_usage += disk_change;
    if (disk_usage < 0) disk_usage = 0;
    if (disk_usage > 1) disk_usage = 1;

    char disk_text[50];
    snprintf(disk_text, sizeof(disk_text), "Disk: %.2f GB Used / %.2f GB Free", disk_usage * 1024, (1 - disk_usage) * 1024);
    gtk_label_set_text(GTK_LABEL(data->disk_label), disk_text);

    
    for (int i = 0; i < 99; ++i) {
        data->disk_usage_data[i] = data->disk_usage_data[i + 1];
    }
    data->disk_usage_data[99] = disk_usage;

    int total_processes = get_total_processes();
    if (total_processes == -1) {
        fprintf(stderr, "Failed to get total number of processes\n");
        return G_SOURCE_CONTINUE;
    }
    ProcessInfo *processes = malloc(total_processes * sizeof(ProcessInfo));
    if (processes == NULL) {
        perror("Error allocating memory");
        return G_SOURCE_CONTINUE;
    }
    DIR *dir = opendir("/proc");
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            double process_cpu_usage = get_process_cpu_usage(pid);
            if (process_cpu_usage != -1) {
                processes[count].pid = pid;
                processes[count].cpu_usage = process_cpu_usage;
                sprintf(processes[count].name, "/proc/%d/stat", pid); 
                count++;
            }
        }
    }
    closedir(dir);

    qsort(processes, total_processes, sizeof(ProcessInfo), compare_process_info);
    printf("PID\tCPU Usage\tProcess Name\n");
    for (int i = 0; i < total_processes; i++) {
      
        FILE *stat_fp = fopen(processes[i].name, "r");
        if (stat_fp != NULL) {
            char process_name[256];
            
            if (fscanf(stat_fp, "%*d (%[^)])", process_name) == 1) {
                printf("%d\t%.2f\t\t%s\n", processes[i].pid, processes[i].cpu_usage, process_name);
            } else {
                printf("%d\t%.2f\t\t[Unknown]\n", processes[i].pid, processes[i].cpu_usage);
            }
            fclose(stat_fp);
        } else {
            printf("%d\t%.2f\t\t[Error opening file]\n", processes[i].pid, processes[i].cpu_usage);
        }
    }

    free(processes);
        
    gtk_widget_queue_draw(data->cpu_chart);
    gtk_widget_queue_draw(data->mem_chart);
    gtk_widget_queue_draw(data->disk_chart);


    return G_SOURCE_CONTINUE;
}

int main(int argc, char *argv[]) {
    MonitorData data;
    initialize_gui(&data);  
    g_timeout_add(1000, update_data, &data);
    gtk_main();

    return 0;
}
