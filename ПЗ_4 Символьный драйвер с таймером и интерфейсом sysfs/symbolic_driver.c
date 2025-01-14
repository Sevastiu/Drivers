#include <linux/module.h>    
#include <linux/kernel.h>   
#include <linux/init.h>     
#include <linux/timer.h>    
#include <linux/fs.h>        
#include <linux/uaccess.h>   
#include <linux/slab.h>     

#define DEVICE_NAME "symbolic_driver" // Имя устройства
#define CLASS_NAME "symbolic"          // Имя класса устройства

MODULE_LICENSE("GPL");                // Лицензия модуля
MODULE_DESCRIPTION("Символьный драйвер с таймером и интерфейсом sysfs"); // Описание модуля

static int global_variable = 0;       // Глобальная переменная для хранения значения
static struct timer_list my_timer;    // Таймер для периодического обновления переменной
static bool timer_running = false;     // Флаг, указывающий, запущен ли таймер

// Функция обратного вызова для таймера
static void timer_callback(struct timer_list *t) {
    global_variable++; // Увеличиваем глобальную переменную
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000)); // Перезапускаем таймер на 1 секунду
}

// Функция чтения значения глобальной переменной
static ssize_t read_variable(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    char buf[32]; // Буфер для хранения строки с значением
    int ret;

    // Форматируем строку с текущим значением глобальной переменной
    ret = snprintf(buf, sizeof(buf), "%d\n", global_variable);
    return simple_read_from_buffer(buffer, len, offset, buf, ret); // Читаем из буфера в пользовательское пространство
}

// Функция для запуска таймера
static ssize_t start_timer(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    if (!timer_running) { // Проверяем, запущен ли таймер
        timer_running = true; // Устанавливаем флаг, что таймер запущен
        mod_timer(&my_timer, jiffies + msecs_to_jiffies(1000)); // Запускаем таймер
    }
    return len; // Возвращаем количество байт, записанных в буфер
}

// Функция для остановки таймера
static ssize_t stop_timer(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    if (timer_running) { // Проверяем, запущен ли таймер
        del_timer(&my_timer); // Останавливаем таймер
        timer_running = false; // Сбрасываем флаг
    }
    return len; // Возвращаем количество байт, записанных в буфер
}

// Функция для сброса глобальной переменной
static ssize_t reset_variable(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    global_variable = 0; // Сбрасываем значение глобальной переменной
    return len; // Возвращаем количество байт, записанных в буфер
}

// Структура операций с файлами
static struct file_operations fops = {
    .owner = THIS_MODULE, // Указываем, что владелец — текущий модуль
    .read = read_variable, // Указываем функцию чтения
    .write = start_timer,  // Указываем функцию записи для запуска таймера
};

// Указатель на объект kobject для создания интерфейса sysfs
static struct kobject *example_kobj;

// Функция инициализации модуля
static int __init symbolic_driver_init(void) {
    // Создаем kobject для интерфейса sysfs
    example_kobj = kobject_create_and_add(DEVICE_NAME, kernel_kobj);
    if (!example_kobj) {
        return -ENOMEM; // Возвращаем ошибку, если не удалось создать kobject
    }

    // Создаем записи в sysfs для управления таймером и чтения переменной
    sysfs_create_file(example_kobj, &fops); // Для чтения переменной
    sysfs_create_file(example_kobj, &fops); // Для запуска таймера
    sysfs_create_file(example_kobj, &fops); // Для остановки таймера
    sysfs_create_file(example_kobj, &fops); // Для сброса переменной

    // Инициализируем таймер
    timer_setup(&my_timer, timer_callback, 0);
    return 0; // Возвращаем 0 для успешной инициализации
}

// Функция очистки модуля
static void __exit symbolic_driver_exit(void) {
    del_timer(&my_timer); // Останавливаем таймер при выгрузке модуля
    kobject_put(example_kobj); // Освобождаем kobject
}

// Указываем функции инициализации и очистки модуля
module_init(symbolic_driver_init);
module_exit(symbolic_driver_exit);