# Mission
Тестовое задание от одного из геймразработчиков. Сыграло свою роль ещё до публикации и теперь используется в перфекционистских экспериментах.

>###Управление
> **ЛКМ** задаёт целевую клетку  
> **ПКМ** создаёт/удаляет препятствия  
> **F11** переключает режим окна  

###Правила
Нужно добратья до выхода, помеченного крестиком. Кроме персонажа всё шевелящееся - враги.

###Сборка
Конфигурируется cmake. На Windows предварительно необходимо разместить собранную SFML в каталоге thrdparty так, чтобы были действительными, как минимум, пути thrdparty\SFML\include и thrdparty\SFML\lib. На юникс-производных системах SFML должна найтись без дополнительных действий, если её пакет разработки установлен.
