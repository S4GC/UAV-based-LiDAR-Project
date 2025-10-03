import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# Parámetros
R = 1.0
n = 10   # resolución angular -> total ~ n^2 puntos
frames = n * n

# -------------------------------
# 1. Esféricas estándar (arriba->abajo, izq->der)
# -------------------------------
theta = np.linspace(-np.pi/2, np.pi/2, n)      # polar
phi = np.linspace(0, np.pi, n)          # azimutal 0-180°
psi = np.linspace(0, 2*np.pi, 2*n)  # azimutal 0-360°
gamma = np.linspace(0, 0.5*np.pi, n//2)  # giro sobre el eje

theta, phi = np.meshgrid(theta, phi)
psi, gamma = np.meshgrid(psi, gamma)

x1 = R * np.sin(theta) * np.cos(phi)
y1 = R * np.sin(theta) * np.sin(phi)
z1 = R * np.cos(theta)
points1 = np.c_[x1.ravel(), y1.ravel(), z1.ravel()]

x2 = R * np.cos(phi) * np.cos(theta)
y2 = R * np.cos(phi) * np.sin(theta)
z2 = R * np.sin(phi)
points2 = np.c_[x2.ravel(), y2.ravel(), z2.ravel()]

x3 = R * np.cos(gamma) * np.cos(psi)
y3 = R * np.cos(gamma) * np.sin(psi)
z3 = R * np.sin(gamma)
points3 = np.c_[x3.ravel(), y3.ravel(), z3.ravel()]

# -------------------------------
# Configuración de gráficas
# -------------------------------
fig = plt.figure(figsize=(12, 4))
axes = [fig.add_subplot(131, projection="3d"),
        fig.add_subplot(132, projection="3d"),
        fig.add_subplot(133, projection="3d")]

titles = ["Esféricas estándar (θ,φ)", "Proyección polar (u,v)", "Latitud-Longitud (λ,φ)"]
data = [points1, points2, points3]

scatters = []
for ax, title in zip(axes, titles):
    ax.set_xlim(-R, R)
    ax.set_ylim(-R, R)
    ax.set_zlim(0, R)
    ax.set_title(title)
    ax.view_init(elev=25, azim=35)
    
    line, = ax.plot([], [], [], linestyle=':', marker='o', color='Darkblue')
    scatters.append(line)
    
# -------------------------------
# Función de animación
# -------------------------------
def update(frame):
    for line, pts in zip(scatters, data):
        subset = pts[:frame]
        line.set_data(subset[:,0], subset[:,1])
        line.set_3d_properties(subset[:,2])
    return scatters

anim = FuncAnimation(fig, update, frames=frames, interval=50, blit=False)

# Guardar como GIF
anim.save("semiesferas_patrones.gif", writer="pillow")
plt.show()

