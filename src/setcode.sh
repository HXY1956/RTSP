sudo apt update
sudo apt install wget gpg

wget -qO- https://packages.microsoft.com/keys/microsoft.asc \
| gpg --dearmor \
| sudo tee /usr/share/keyrings/packages.microsoft.gpg >/dev/null

echo "deb [arch=arm64 signed-by=/usr/share/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" \
| sudo tee /etc/apt/sources.list.d/vscode.list

sudo apt update
sudo apt install code
