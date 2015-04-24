# u3d_function_module
Данный модуль позволяет создавать и уничтожать объекты мира в Virtual Scene(VS).<br>
Для создания доступны 2 объекта: "куб" и сфера.

####Особенности входящих значений для функций модуля:
Переменная **color** должна принимать строковое значение в виде трёх пар шестнадцатеричных цифр(0-F),<br> где каждая пара отвечает за свой цвет, красный, зеленый и синий соответственно. <br>
Остальные переменные принимают численное значение.

####Доступные функции:
Определение  | Описание 
------------  | ----------------- 
createCube(**x**, **y**, **z**, **dx**, **dy**, **dz**, **color**)  | Создает куб цвета **color** в координатах **x**,**y**,**z** длиной шириной и высотой в **dx**,**dy**,**dz**. Возвращает **object_id** созданного объекта.
createSphere(**x**, **y**, **z**, **R**, **color**)  | Создает сферу цвета **color** с цетром в координатах **x**,**y**,**z** радиуса **R**. Возвращает **object_id** созданного объекта.
deleteCreatedObject(**object_id**)  | Удаляет объект с заданным **object_id**

###### Ответственность за удаление объектов окружения возлагается на программиста RCML.
