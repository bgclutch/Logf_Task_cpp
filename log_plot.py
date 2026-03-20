import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Загружаем данные (предполагаем заголовки: x,my_logf,std_log,ulp_error)
df = pd.read_csv('analysis.csv')

plt.figure(figsize=(12, 6))

# Рисуем обе линии
plt.plot(df['x'], df['std::log'], label='std::log (Reference)', color='blue', lw=2, linestyle='--')
plt.plot(df['x'], df['logf'], label='logf (Mine)', color='red', lw=1, alpha=0.7)

plt.title('Comparison: logf and std::log')
plt.xlabel('x')
plt.ylabel('log(x)')
plt.legend()
plt.grid(True, which="both", ls="-", alpha=0.5)

plt.savefig('comparison.png')
plt.show()