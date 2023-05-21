# ИДЗ 3. Вариант 15. Выполнил Федоров Артём Олегович, БПИ217.

**Программа выполнена на 9 баллов**

## Условие задчи:

Задача 15. O клумбе – 1. На клумбе растет 40 цветов, за ними непрерывно следят два процесса–садовника 
и поливают увядшие цветы, при этом оба садовника очень боятся полить один и тот же цветок, 
который еще не начал вянуть. <br/>
Создать приложение, моделирующее состояния цветков на клумбе и действия садовников. 
Для изменения состояния цветов создать отдельный процесс (а не 40 процессов для каждого цветка), 
который может задавать одновременное начало увядания для нескольких цветков.

**Заметка 1**:
Я сделал клумбу из 20 цветов, так как информации, выводимой во времени работы программы, слишком много и тяжело за всем уследить,
Поэтому мной было принято логичное решение, которое никак не повлияло на явное упрощение задачи, 
сделать клумбу из 20, а не 40 цветов.  <br/>
(Можете заменить 3 define в самом начале программы: Кол-во цветов на 40 и длину + ширину клумба для вывода) <br/>
Будет только хуже из-за большого числа строк вывода, но при этом программа также прекрасно отработает.

## Запуск

Аргументы для Сервера:

1. port - например: 8080 (по умолчанию port = 8080)
2. n - количество дней в саду (по умолчанию n = 3)
3. seed - сид для случайной генерации (по умолчанию фиксированный = 42)

Аргументы для Клиентов:

1. port сервера - например: 8080 (по умолчанию port = 8080)
2. ip сервера - например: 127.0.0.1

Пример запуска программы:

Первый терминал:
> ./server 8080 2 42

Второй терминал:
> ./client 8080 127.0.0.1

## Отчёт на 4-5:

[Код программы](https://github.com/ArtemFed/Operating-Systems-HW3/tree/main/score4-5)

#### 1. Сценарий:
*  Сад с 40(20) цветами только посадили и все цветы политы - Сервер.
*  Начинается цикл по дням (за этим следит процесс природы - отдельный поток на сервере)
*  Каждую ночь цветы начинают увядать, если они проведут ночь в состоянии увядания, 
то они погибнут без возможности воскрешения.
*  Утром два Садовника - Клиента просыпаются и начинают поливать цветы, 
они имеют ограниченный запас воды в лейках, поэтому некоторым цветам не хватит воды, 
и они завянут.
*  Работа садовников и сервера:
* * Клиент-садовник отправляет свой случайный номер серверу (при запуске) 
  * Каждый клиент-садовник направляет по TCP n / 2 значений (индексов цветов)
  * Сервер-клумба принимает воду, впитывает и возвращает один из ответов:
    * Цветок полит
    * Цветок уже был полит
    * Цветок уже завял
  * Клиент получает ответ и выводит его
  * Всё повторяется пока дни не закончатся
*  Ночью Садовники спят, а цветы вянут.
*  Дни кончились, наступает Армагеддон, всё стирается и уничтожается

#### 2. Проверено как на одном, так и на двух компютерах в одной сети

**Пример исполнения на 4-5 баллов**
1. Один день, стандартный сид = 42
2. Садовники запускаются сразу же и включаются в работу <br/>

![hw3_4-5](https://github.com/ArtemFed/Operating-Systems-HW3/assets/57373162/2cb04de1-e0fa-45bc-973b-70afb4e6f24c)


----


## Отчёт на 6-7

[Код программы](https://github.com/ArtemFed/Operating-Systems-HW3/tree/main/score6-8)


#### 1. Специфика: <br/>
Теперь к программе добавлен файл [viewer.c](https://github.com/ArtemFed/Operating-Systems-HW3/tree/main/score6-8/viewer.c): <br/>
Viewer отвечает за удаленный просмотр действий сервера и выводит информацию о клумбе в различные моменты времени

Его необходимо запускать так:
> ./viewer 8080 127.0.0.1

**Пример исполнения на 6-7 баллов**  <br/>

1.  2 дня
2.  "Верхний" viewer подключается сразу, после второго дня завершается с "Ctrl+C" и программа продолжает корректно работать 
3.  "Нижний" viewer подключается на второй день 

![hw3_6-8](https://github.com/ArtemFed/Operating-Systems-HW3/assets/57373162/bcc8ba5f-5dc1-461f-b2f2-84c5c202d1e0)


----


## Отчёт на 8

[Код программы](https://github.com/ArtemFed/Operating-Systems-HW3/tree/main/score6-8)

#### 1. Специфика: <br/>
Добавлена возможность подключения множества клиентов-наблюдателей, они могут быть подключены и отключеный в любой момент
Аккуратно завершаются вместе с окончанием работы сервера

![hw3_6-8](https://github.com/ArtemFed/Operating-Systems-HW3/assets/57373162/bcc8ba5f-5dc1-461f-b2f2-84c5c202d1e0)


----


## Отчёт на 9

[Код программы](https://github.com/ArtemFed/Operating-Systems-HW3/tree/main/score6-8)

Добавлена возможность подключать и отключать обычных клиентов-садовников в любой момент, как и отключать

Это можно наблюдать на данном примере
1. 2 дня
2. "Верхний" клиент-садовник №34 подключается на первый день и отключается с "Ctrl+C"
3. Программа работает дальше и "Нижний" клиент-садовник №28 завершает полив цветов
4. Сервер отработал как нужно

![hw3_9](https://github.com/ArtemFed/Operating-Systems-HW3/assets/57373162/351feec4-adf4-48d9-bb47-a63d94e69dcd)
