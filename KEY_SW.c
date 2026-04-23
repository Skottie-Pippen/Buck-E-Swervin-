# ⚡ Buck-E Swervin' - Quick Start

**Get the game running in under 2 minutes!**

---

## 🎯 5-Step Launch Sequence

### Step 1: Load Board Drivers
```bash
cd /home/root/Linux_Libraries/drivers
./load_drivers
```

### Step 2: Remove Default Video Driver
```bash
rmmod video.ko
```

### Step 3: Build the Game
```bash
cd /path/to/Buck-E-Swervin/code
make
```

### Step 4: Load Custom Video Driver
```bash
insmod video.ko
```

### Step 5: Run!
```bash
./bucke_swervin
```

---

## 🎮 Controls

| Button | Action |
|--------|--------|
| **KEY0** | Move Right / Start Game |
| **KEY1** | Move Left |
| **SW[1:0]** | Difficulty (00=Easy, 11=Xtreme) |

---

## 🛑 To Exit

1. Press `Ctrl+C` in terminal
2. Unload driver: `rmmod video.ko`

---

**That's it! Dodge defenders and rack up points! 🏈**

For full documentation, see [README.md](README.md)
