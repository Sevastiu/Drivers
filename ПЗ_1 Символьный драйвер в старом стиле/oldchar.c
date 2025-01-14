#include <linux/module.h>    // Основные заголовки модуля
#include <linux/fs.h>        // Файловая система
#include <linux/uaccess.h>   // Для копирования данных из/в пользовательскую область
#include <linux/slab.h>      // Для динамического выделения памяти

#define DEVICE_NAME "oldchardev" // Имя устройства
#define BUF_LEN 80               // Длина буфера

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Символьный драйвер в старом стиле");

static int major;                // Основной номер устройства
static char message[BUF_LEN];    // Буфер для хранения сообщений
static struct class*  oldcharClass  = NULL; // Класс устройства
static struct device* oldcharDevice = NULL; // Устройство

// Прототипы функций
static int     oldchar_open(struct inode *, struct file *);
static int     oldchar_release(struct inode *, struct file *);
static ssize_t oldchar_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t oldchar_write(struct file *, const char __user *, size_t, loff_t *);

// Описание операций файла
static struct file_operations fops =
{
    .open = oldchar_open,
    .read = oldchar_read,
    .write = oldchar_write,
    .release = oldchar_release,
};

// Функция инициализации модуля
static int __init oldchar_init(void)
{
    // Регистрация устройства
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Не удалось зарегистрировать устройство: %d\n", major);
        return major;
    }

    // Создание класса устройства
    oldcharClass = class_create(THIS_MODULE, "chardrv");
    if (IS_ERR(oldcharClass)) {
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "Не удалось создать класс устройства\n");
        return PTR_ERR(oldcharClass);
    }

    // Создание устройства
    oldcharDevice = device_create(oldcharClass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(oldcharDevice)) {
        class_destroy(oldcharClass);
        unregister_chrdev(major, DEVICE_NAME);
        printk(KERN_ALERT "Не удалось создать устройство\n");
        return PTR_ERR(oldcharDevice);
    }

    printk(KERN_INFO "Устройство %s успешно зарегистрировано с номером %d\n", DEVICE_NAME, major);
    return 0;
}

// Функция очистки модуля
static void __exit oldchar_exit(void)
{
    device_destroy(oldcharClass, MKDEV(major, 0)); // Уничтожение устройства
    class_unregister(oldcharClass);                 // Отмена регистрации класса
    class_destroy(oldcharClass);                    // Уничтожение класса
    unregister_chrdev(major, DEVICE_NAME);         // Отмена регистрации устройства
    printk(KERN_INFO "Устройство %s успешно удалено\n", DEVICE_NAME);
}

// Функция открытия устройства
static int oldchar_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Устройство %s открыто\n", DEVICE_NAME);
    return 0;
}

// Функция закрытия устройства
static int oldchar_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Устройство %s закрыто\n", DEVICE_NAME);
    return 0;
}

// Функция чтения из устройства
static ssize_t oldchar_read(struct file *file, char __user *buffer, size_t len, loff_t *offset)
{
    int bytes_read = 0;

    // Если нет данных для чтения
    if (*message == 0) {
        return 0;
    }

    // Копирование данных в пользовательский буфер
    while (len && *message) {
        put_user(*(message++), buffer++);
        len--;
        bytes_read++;
    }

    printk(KERN_INFO "Прочитано %d байт(ов) из устройства %s\n", bytes_read, DEVICE_NAME);
    return bytes_read;
}

// Функция записи в устройство
static ssize_t oldchar_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset)
{
    int i;
    for (i = 0; i < len && i < BUF_LEN; i++) {
        get_user(message[i], buffer + i);
    }
    message[i] = '\0'; // Завершение строки
    printk(KERN_INFO "Записано в устройство %s: %s\n", DEVICE_NAME, message);
    return i;
}

module_init(oldchar_init);  // Инициализация модуля
module_exit(oldchar_exit);  // Очистка модуля

