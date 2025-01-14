#include <linux/kernel.h>    // Для использования функций ядра, таких как printk
#include <linux/module.h>    // Основные заголовки модуля
#include <linux/netdevice.h> // Для работы с сетевыми устройствами
#include <linux/interrupt.h> // Для обработки прерываний

// Обработчик прерываний для входящих пакетов
static irqreturn_t my_interrupt_handler(int irq, void *dev_id) {
    struct net_device *dev = dev_id; // Получаем указатель на сетевое устройство

    // Логируем номер прерывания
    printk(KERN_INFO "Interrupt received: %d\n", irq);

    // Логируем адрес области ввода-вывода
    printk(KERN_INFO "I/O address: %p\n", dev->base_addr);

    // Логируем размер области ввода-вывода
    printk(KERN_INFO "I/O area size: %zu\n", sizeof(dev->base_addr));

    return IRQ_HANDLED; // Возвращаем, что прерывание обработано
}

// Функция обработки входящих пакетов
static int my_receive_packet(struct sk_buff *skb, struct net_device *dev) {
    // Логируем содержимое входящего пакета и его размер
    printk(KERN_INFO "Received packet size: %zu\n", skb->len);
    // Дополнительная логика обработки пакета может быть добавлена здесь
    return 0; // Возвращаем 0 для успешной обработки
}

// Функция обработки остановки сетевого устройства
static int cpsw_ndo_stop(struct net_device *ndev) {
    printk(KERN_INFO "cpsw_ndo_stop called"); // Логируем вызов функции остановки

    struct cpsw_priv *priv = netdev_priv(ndev); // Получаем приватные данные устройства
    struct cpsw_common *cpsw = priv->cpsw; // Получаем общие данные CPSW
    cpsw->usage_count--; // Уменьшаем счетчик использования

    pm_runtime_put_sync(cpsw->dev); // Освобождаем ресурсы

    // Логируем информацию об области ввода-вывода
    printk(KERN_INFO "IO region: %lx, size: %d\n", ndev->base_addr, ndev->mem_end - ndev->base_addr);
    return 0; // Возвращаем 0 для успешной остановки
}

// Функция обработки открытия сетевого устройства
static int cpsw_ndo_open(struct net_device *ndev) {
    printk(KERN_INFO "cpsw_ndo_open called"); // Логируем вызов функции открытия

    struct cpsw_priv *priv = netdev_priv(ndev); // Получаем приватные данные устройства
    struct cpsw_common *cpsw = priv->cpsw; // Получаем общие данные CPSW
    int ret;

    // Дополнительная логика открытия устройства может быть добавлена здесь
    return 0; // Возвращаем 0 для успешного открытия
}

// Функция обработки передачи пакетов
static netdev_tx_t cpsw_ndo_start_xmit(struct sk_buff *skb, struct net_device *ndev) {
    skb_tx_timestamp(skb); // Устанавливаем временную метку для пакета

    // Логируем информацию об области ввода-вывода перед передачей
    printk(KERN_INFO "IO region: %lx, size: %d\n", ndev->base_addr, ndev->mem_end - ndev->base_addr);

    // Логируем содержимое исходящего пакета и его размер
    printk(KERN_INFO "Outgoing packet data: %p, size: %d\n", skb->data, skb->len);

    // Логика передачи пакета
    ret = cpdma_chan_submit(txch, skb, skb->data, skb->len, priv->emac_port);
    if (unlikely(ret != 0)) {
        cpsw_err(priv, tx_err, "desc submit failed\n"); // Логируем ошибку передачи
        goto fail; // Переходим к обработке ошибки
    }

    return NETDEV_TX_OK; // Возвращаем статус успешной передачи
}

// Функция инициализации драйвера
static int cpsw_probe(struct platform_device *pdev) {
    printk(KERN_INFO "cpsw_probe called"); // Логируем вызов функции инициализации

    struct cpsw_common *cpsw;
    cpsw = devm_kzalloc(dev, sizeof(struct cpsw_common), GFP_KERNEL); // Выделяем память для структуры CPSW
    if (!cpsw)
        return -ENOMEM; // Возвращаем ошибку, если память не выделена

    // Получаем номер прерывания для RX
    int irq = platform_get_irq_byname(pdev, "rx");
    if (irq < 0)
        return irq; // Возвращаем ошибку, если не удалось получить IRQ
    cpsw->irqs_table[0] = irq; // Сохраняем номер IRQ для RX

    printk(KERN_INFO "IRQ rx number: %d\n", irq); // Логируем номер IRQ для RX

    // Получаем номер прерывания для TX
    irq = platform_get_irq_byname(pdev, "tx");
    if (irq < 0)
        return irq; // Возвращаем ошибку, если не удалось получить IRQ
    cpsw->irqs_table[1] = irq; // Сохраняем номер IRQ для TX

    printk(KERN_INFO "IRQ tx number: %d\n", irq); // Логируем номер IRQ для TX

    // Получаем номер прерывания для других событий
    irq = platform_get_irq_byname(pdev, "misc");
    if (irq <= 0)
        return irq; // Возвращаем ошибку, если не удалось получить IRQ
    cpsw->misc_irq = irq; // Сохраняем номер IRQ для других событий

    printk(KERN_INFO "IRQ misc number: %d\n", irq); // Логируем номер IRQ для других событий

    platform_set_drvdata(pdev, cpsw); // Сохраняем данные драйвера в платформенном устройстве
    return 0; // Возвращаем 0 для успешной инициализации
}