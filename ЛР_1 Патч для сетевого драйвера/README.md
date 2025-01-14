
# ЛР_1 Патч для сетевого драйвера

## Задание
Разработать патч для сетевого драйвера Beaglebone Black. Патч должен обеспечивать печать в log ядра:
•	номера прерываний драйвера;
•	адрес (физический) области ввода-вывода;
•	размер области;
•	содержимое и размер данных входящих и исходящих пакетов.


## Описание
Изменения в коде:
### Логирование входящих пакетов
```
printk(KERN_INFO "Incoming packet data: %p, size: %d\n", skb->data, skb->len);
```

### Логирование остановки устройства

```
printk(KERN_INFO "cpsw_ndo_stop called");
```
### Логирование информации об области ввода-вывода
```
printk(KERN_INFO "IO region: %lx, size: %d\n", ndev->base_addr, ndev->mem_end - ndev->base_addr);
```
### Логирование исходящих пакетов
```
printk(KERN_INFO "Outgoing packet data: %p, size: %d\n", skb->data, skb->len);
```
### Логирование прерываний
```
printk(KERN_INFO "IRQ rx number: %d\n", irq);
printk(KERN_INFO "IRQ tx number: %d\n", irq);
printk(KERN_INFO "IRQ misc number: %d\n", irq);
```