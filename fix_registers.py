#!/usr/bin/env python3
import re

# Читаем файл
with open('src/registers_config.h', 'r') as f:
    content = f.read()

# Заменяем проблемные значения
# 1. Заменяем 0xFFFFFFFF на 999999999.0f
content = re.sub(r'0xFFFFFFFF', '999999999.0f', content)

# 2. Добавляем суффикс f к целочисленным значениям в float контексте
# Это сложнее, но давайте просто заменим самые проблемные места
content = re.sub(r', 0, 0xFFFFFFFF, 0', ', 0, 999999999.0f, 0', content)
content = re.sub(r', 0, 0xFFFFFFFF', ', 0, 999999999.0f', content)

# Записываем обратно
with open('src/registers_config.h', 'w') as f:
    f.write(content)

print('Fixed registers_config.h')