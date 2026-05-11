# 🚪 Doors for the 3DS

This is a fan demake of the Roblox horror game **Doors** which was originally made by LSPLASH. This is being built completely from the ground up as obviously, there is no translation from Luau (Roblox code) to C++ (3DS code). 

I enjoy playing Doors, and since there aren't really many good horror games for the 3DS, I decided to make this for fun and to see just how far I can push the 3DS hardware.

---

## 🛠 Currently Implemented

### **The Monsters**
- [x] **Rush** – Flickers the lights and requires you to hide.
- [x] **Seek** – Full chase sequence using the music from the original game, custom cinematic transitions, and dynamic FOV.
- [x] **Screech** – Spawns in dark rooms; you have to look at him when he says "Psst!"
- [x] **Eyes** – Deals damage if you look directly at him.
- [x] **Dupe** – Fake doors that require you to check the room number plaques to find the right path.

### **Gameplay**
- [x] **Exploration** – Procedural hallways and side rooms with paintings and furniture.
- [x] **Looting** – Interactable chests for loot and drawers that can be opened for items.
- [x] **Hiding** – Functional wardrobes and beds to stay safe.
- [x] **Controls** – Native C-Stick support (New 3DS), Touchpad support (Old 3DS), and L & R shoulder button turning.

---

## 📝 Planned for Future Updates
- [ ] **Door Numbers** – Adding actual numbers to the doors instead of just having the room number on the bottom screen.
- [ ] **Figure** – Finishing the blind AI and the full Library encounter.
- [ ] **Room Generation** – Making the layout less linear and adding specific room types.
- [ ] **The Shop** – I'm planning to add both a menu-based shop and the in-game Jeff shop.
- [ ] **More Entities** – Timothy, Ambush, Hide, Halt, Dread, and Glitch.
- [ ] **Multiplayer** – Support for playing through the hotel with others.
- [ ] **3D Models** – *(Maybe)* Moving from flat sprites to full 3D models for the monsters.

---

## 🎮 Controls
| Input | Action |
| :--- | :--- |
| **Circle Pad** | Move |
| **C-Stick / Touchpad / L & R** | Look Around |
| **Button A** | Interact (Open doors, pick up loot, unlock doors) |
| **Button B** | Crouch |
| **Button X** | Open Drawers / Hide (Wardrobes and beds) |

---

## 📦 Download & Community
* **Download**: Grab the latest `.cia` or `.3dsx` on the [Releases](../../releases) page, or scan this QR code with FBI
<img width="270" height="270" alt="image" src="https://github.com/user-attachments/assets/3499232f-2da2-460d-8f06-968cb0b60cf9" />

* **Discord**: [Join the server](https://discord.gg/h5JVJbSyu8) for updates, technical help, and suggestions. 

---

## 💬 FAQ

**Why did you make this?**

I’m a fan of the original game and noticed that the 3DS is pretty short on good horror titles. I wanted to see if I could push the hardware to its limits while making something I actually enjoyed playing. It’s been a great project for learning and seeing just how much this console can handle.

**The game has no sound / "Missing DSP Firmware" error.**

You just need to dump your system's sound firmware. Run the **DSP1** homebrew app on your 3DS once; it will create the `dspfirm.cdc` file on your SD card, and audio will work fine after that.

**Why is the game locked at 30 FPS?**

The 3DS has a very limited amount of RAM. To keep the textures high-quality and the room generation stable without the game crashing, 30 FPS is the most reliable target.

**What do the coins do?**

Right now, they are just for collecting. A shop system is planned so you can actually spend them later on!

---

## 🌟 Credits
* **Original Game & Concept**: LSPLASH / Roblox Team.
* **Music & Sound Effects**: LSPLASH (all music and sound assets used are from the original game).
* **3DS Development**: Carmander152.
* **Tools**: devkitPro, Citro3D, Tex3DS.
