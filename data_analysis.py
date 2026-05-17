import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

# =========================
# 1. Carregar arquivo
# =========================
excel_path = 'dados.xlsx'

if not os.path.exists(excel_path):
    raise FileNotFoundError("Arquivo 'dados.xlsx' não encontrado.")

df = pd.read_excel(excel_path, header=1)

# Ajustar nomes das colunas
expected = ['D_real', 'Distancia_FTM', 'RSSI']

if list(df.columns[:3]) != expected:
    df.columns = expected + list(df.columns[3:])

# Converter para valores numéricos
for col in expected:
    df[col] = pd.to_numeric(df[col], errors='coerce')

# Remover linhas inválidas
df = df.dropna(subset=['D_real', 'Distancia_FTM'])

# =========================
# 2. Remover Outliers (IQR)
# =========================
Q1 = df['Distancia_FTM'].quantile(0.25)
Q3 = df['Distancia_FTM'].quantile(0.75)

IQR = Q3 - Q1

limite_inferior = Q1 - 1.5 * IQR
limite_superior = Q3 + 1.5 * IQR

df_filtrado = df[
    (df['Distancia_FTM'] >= limite_inferior) &
    (df['Distancia_FTM'] <= limite_superior)
]

print(f'Total original: {len(df)} amostras')
print(f'Total sem outliers: {len(df_filtrado)} amostras')

# =========================
# 3. Regressão Linear
# =========================
slope, intercept, r_value, p_value, std_err = stats.linregress(
    df_filtrado['D_real'],
    df_filtrado['Distancia_FTM']
)

r2 = r_value ** 2

# =========================
# 4. Configuração IEEE
# =========================
plt.rcParams.update({
    'font.family': 'serif',
    'font.serif': ['Times New Roman'],

    # Fontes maiores para IEEE
    'font.size': 20,
    'axes.labelsize': 20,
    'xtick.labelsize': 20,
    'ytick.labelsize': 20,
    'legend.fontsize': 13,

    'axes.grid': True,
    'grid.alpha': 0.3,
})

# Figura maior
fig, ax = plt.subplots(figsize=(4.5, 3.6))

# =========================
# 5. Scatter Plot
# =========================
ax.scatter(
    df_filtrado['D_real'],
    df_filtrado['Distancia_FTM'],
    color='#1f77b4',
    alpha=0.75,
    s=45,
    label='Medições FTM'
)

# =========================
# 6. Linha ideal
# =========================
x = np.linspace(
    df_filtrado['D_real'].min(),
    df_filtrado['D_real'].max(),
    100
)

ax.plot(
    x,
    x,
    linestyle='--',
    color='black',
    linewidth=1.5,
    label='Linha Ideal ($y=x$)'
)

# =========================
# 7. Ajuste linear
# =========================
y_fit = slope * x + intercept

ax.plot(
    x,
    y_fit,
    color='red',
    linewidth=1.5,
    label=f'Ajuste Linear ($R^2={r2:.2f}$)'
)

# =========================
# 8. Labels
# =========================
ax.set_xlabel('Distância Real (m)')
ax.set_ylabel('Distância Estimada por FTM (m)')

# =========================
# 9. Legenda
# =========================
ax.legend(loc='upper left', frameon=True)

# =========================
# 10. Layout
# =========================
plt.tight_layout()

# =========================
# 11. Exportar
# =========================
output_pdf = 'grafico_ftm_sem_outliers.pdf'
output_png = 'grafico_ftm_sem_outliers.png'

plt.savefig(output_pdf, dpi=300, bbox_inches='tight')
plt.savefig(output_png, dpi=300, bbox_inches='tight')

print('\nGráficos salvos:')
print(f' - {output_pdf}')
print(f' - {output_png}')

plt.show()