#include <linux/init.h>      
#include <linux/module.h>     
#include <linux/version.h>    
#include <linux/sched.h>         
#include <linux/kthread.h>
#include <linux/kernel.h>      
#include <linux/delay.h>        
#include <linux/moduleparam.h>  

MODULE_LICENSE("GPL");          // Лицензия модуля: GNU Public License.
MODULE_DESCRIPTION("Модуль ядра с параметрами частоты и сообщения");

static struct task_struct *task; // Указатель на поток
static int frequency = 1; // Частота в секундах
static char *message = "Привет, мир!"; // Сообщение по умолчанию

// Определение параметров модуля
module_param(frequency, int, S_IRUGO);
MODULE_PARM_DESC(frequency, "Частота вывода сообщения в секундах");

module_param(message, charp, S_IRUGO);
MODULE_PARM_DESC(message, "Сообщение для вывода");

static int thread_fn(void *data) {
    // Функция потока, которая будет выполняться
    while (!kthread_should_stop()) {
        printk(KERN_INFO "Сообщение от модуля: %s\n", message);
        ssleep(frequency); // Задержка на заданное количество секунд
    }
    return 0;
}

static int __init sleep_module_init(void) {
    // Инициализация модуля
    task = kthread_run(thread_fn, NULL, "my_thread"); // Запуск потока
    if (IS_ERR(task)) {
        printk(KERN_ALERT "Не удалось создать поток\n");
        return PTR_ERR(task);
    }
    return 0;
}

static void __exit sleep_module_exit(void) {
    // Выгрузка модуля
    if (task) {
        kthread_stop(task); // Остановка потока
    }
    printk(KERN_INFO "Модуль выгружен\n");
}

// Определение функций и лицензии модуля
module_init(sleep_module_init);
module_exit(sleep_module_exit);
