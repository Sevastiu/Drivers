#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h>  
#include <linux/timer.h>  
#include <linux/sched.h> 
#include <linux/pid.h>   
#include <linux/signal.h> 
#include <linux/uaccess.h> 
#include <linux/fs.h>     
#include <linux/slab.h>  
#include <linux/ktime.h> 


MODULE_LICENSE("GPL"); // Лицензия
MODULE_DESCRIPTION("A symbolic Linux driver to measure reaction time"); // Описание

#define DEVICE_NAME "reaction_time_analyzer" // Имя устройства
#define IOCTL_SET_FREQUENCY _IOW('a', 'b', unsigned long) // Команда для установки частоты
#define IOCTL_GET_STATS _IOR('a', 'c', struct stats) // Команда для получения статистики

#define MAX_REACTIONS 1000 // Максимальное количество реакций
#define BINS 10 // Количество бинов для гистограммы

// Структура для хранения статистики
struct stats {
    unsigned long average_time; // Среднее время реакции
    unsigned long max_time; // Максимальное время реакции
    unsigned long histogram[BINS]; // Гистограмма времени реакции
};

static struct timer_list reaction_timer; // Таймер для измерения времени реакции
static unsigned long frequency = 1000; // Частота в миллисекундах
static ktime_t last_signal_time; // Время последнего сигнала
static unsigned long reaction_times[MAX_REACTIONS]; // Массив для хранения времен реакции
static int reaction_count = 0; // Счетчик реакций

// Функция обратного вызова для таймера
static void timer_callback(struct timer_list *t) {
    struct pid *pid_struct;
    struct task_struct *task;

    // Отправка сигнала текущему процессу
    pid_struct = find_get_pid(current->pid); // Получение структуры PID
    if (pid_struct) {
        task = pid_task(pid_struct, PIDTYPE_PID); // Получение структуры задачи по PID
        if (task) {
            send_sig(SIGUSR1, task, 0); // Отправка сигнала SIGUSR1
            last_signal_time = ktime_get_real(); // Обновление времени последнего сигнала
        }
        put_pid(pid_struct); // Освобождение структуры PID
    }

    // Перепланировка таймера
    mod_timer(&reaction_timer, jiffies + msecs_to_jiffies(frequency));
}

// Функция для измерения времени реакции
static void measure_reaction_time(void) {
    ktime_t end_time = ktime_get_real(); // Получение текущего времени
    unsigned long reaction_time = ktime_to_ns(ktime_sub(end_time, last_signal_time)); // Вычисление времени реакции

    // Сохранение времени реакции, если не превышен лимит
    if (reaction_count < MAX_REACTIONS) {
        reaction_times[reaction_count++] = reaction_time; // Сохранение времени реакции в массив
    }
}

// Обработчик IOCTL для управления устройством
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_SET_FREQUENCY:
            // Установка частоты
            if (copy_from_user(&frequency, (unsigned long *)arg, sizeof(unsigned long))) {
                return -EFAULT; // Ошибка при копировании данных из пользовательского пространства
            }
            mod_timer(&reaction_timer, jiffies + msecs_to_jiffies(frequency)); // Перепланировка таймера
            break;
        case IOCTL_GET_STATS: {
            struct stats s = {0}; // Инициализация структуры статистики
            unsigned long total_time = 0; // Переменная для хранения общего времени
            unsigned long max_time = 0; // Переменная для хранения максимального времени
            unsigned long bin_size = 0; // Размер бина для гистограммы
            int i;

            // Вычисление общего времени и максимального времени
            for (i = 0; i < reaction_count; i++) {
                total_time += reaction_times[i];
                if (reaction_times[i] > max_time) {
                    max_time = reaction_times[i]; // Обновление максимального времени
                }
            }

            // Вычисление среднего времени
            s.average_time = reaction_count ? total_time / reaction_count : 0;
            s.max_time = max_time;

            // Построение гистограммы
            bin_size = max_time / BINS; // Определение размера бина
            for (i = 0; i < reaction_count; i++) {
                int bin_index = reaction_times[i] / bin_size; // Определение индекса бина
                if (bin_index >= BINS) {
                    bin_index = BINS - 1; // Последний бин для переполнения
                }
                s.histogram[bin_index]++; // Увеличение счетчика в соответствующем бине
            }

            // Копирование статистики в пользовательское пространство
            if (copy_to_user((struct stats *)arg, &s, sizeof(struct stats))) {
                return -EFAULT; // Ошибка при копировании данных в пользовательское пространство
            }
            break;
        }
        default:
            return -EINVAL; // Неверная команда
    }
    return 0; // Успешное выполнение
}

// Определение операций с файлом
static struct file_operations fops = {
    .unlocked_ioctl = device_ioctl, // Указатель на обработчик IOCTL
};

// Функция инициализации драйвера
static int __init reaction_time_driver_init(void) {
    int ret;

    // Регистрация символьного устройства
    ret = register_chrdev(0, DEVICE_NAME, &fops);
    if (ret < 0) {
        pr_alert("Failed to register character device\n"); // Ошибка регистрации устройства
        return ret;
    }

    // Настройка таймера
    timer_setup(&reaction_timer, timer_callback, 0);
    mod_timer(&reaction_timer, jiffies + msecs_to_jiffies(frequency)); // Запуск таймера

    pr_info("Reaction time driver initialized\n"); // Сообщение об успешной инициализации
    return 0;
}

// Функция выгрузки драйвера
static void __exit reaction_time_driver_exit(void) {
    del_timer(&reaction_timer); // Удаление таймера
    unregister_chrdev(0, DEVICE_NAME); // Удаление символьного устройства
    pr_info("Reaction time driver exited\n"); // Сообщение о выгрузке драйвера
}

// Определение функций и лицензии модуля
module_init(reaction_time_driver_init);
module_exit(reaction_time_driver_exit);

