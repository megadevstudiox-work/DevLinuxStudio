https://www.youtube.com/watch?v=VB2c342Oqx0
https://www.mediafire.com/file/1g2bkmmoyivosb4/devlinux_studio.zip/file 
https://megadevstudiox.netlify.app/
🚀 DevLinuxStudio
DevLinuxStudio is a lightweight, cross-language Linux development suite designed for developers. It features a clean, custom "from scratch" architecture separating UI (Python/Tkinter) from Core engines (C++, C, Rust, Go, Java), complete with real-time output debugging, right-click file/asset management, and automated AppImage packaging.

📜 License
This project is licensed under the terms of the MIT License.

Plaintext
MIT License

Copyright (c) 2026 Mega Dev Studio X

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
⚙️ Prerequisites & System Requirements
Before testing or running DevLinuxStudio, ensure your system meets the following requirements:

Supported Distributions: Ubuntu 24.04 x64+, Debian 13 x64+, Fedora 40+, SteamOS 3.6+ x64, Arch Linux.

Architecture: 64-bit Linux environments only (x86_64).

Required System Dependency (GLFW libraries):
Install the required rendering libraries for your distribution using the command below:

Ubuntu / Debian / Linux Mint:

Bash
sudo apt update && sudo apt install -y libglfw3 libglfw3-dev
💻 How to Run from Source Code
If you downloaded the repository as a .zip file:

Extract the ZIP file to a folder on your machine.

Open a terminal inside the extracted directory.

Build and run the application using your configuration or build files (make).

📦 How to Run the AppImage

Download the DevLinuxStudio.AppImage executable.

Right-click the .AppImage file → Properties → Permissions → Check "Allow executing file as program".

Double-click to launch directly, or run via terminal:

Bash
chmod +x DevLinuxStudio.AppImage
./DevLinuxStudio.AppImage
✨ Core Features of the System

Split Architecture: Organizes projects cleanly into two main folders: UI/ (Python scripts and assets such as images, videos, documents) and Core/ (high-performance code in C++, C, Rust, Go, etc.).

Right-Click File Management:

Inside the UI folder: Right-clicking allows you to create new Python files (.py) or upload external files (photos, videos, documents) directly into the project.

Inside the Core folder: Allows adding new core source files for various programming languages.

Interactive Output Panel (Mouse Copy Support):

Any success message or execution error is displayed in the bottom panel in real-time.

Text can be freely selected with the mouse and easily copied using Ctrl + C.

Automated AppImage Packaging:

With a single command from the build menu, a standalone AppImage package is generated for direct distribution on any 64-bit Linux system.

🤝 Community Contributions & Support
Lots of hard work went into developing this application! As an independent developer, I warmly welcome feedback, bug reports, and contributions from the Linux community.

Report Issues: Open an issue on GitHub if you encounter bugs or compatibility problems.

Contribute: Feel free to fork the repository, refine the codebase, and submit Pull Requests.

Spread the Word: Share your suggestions and ideas to make Linux development even better!

Thank you all for your support! ❤️
