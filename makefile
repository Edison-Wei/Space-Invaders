all: Program

Program: Program.o main.o
	g++ main.o -o main.exe -L/opt/homebrew/Cellar/glfw/3.4/lib -Lglew-2.2.0/lib -lglfw -lglew -framework OpenGL 

# Forcing to compile using std=c++11
Program.o: main.cpp Include.h Alien.h Buffer.h Player.h Projectile.h Sprite.h SpriteAnimation.h
	g++ -c -std=c++11 main.cpp -I/opt/homebrew/Cellar/glfw/3.4/include -Iglew-2.2.0/include 

clean:
	rm *.exe *.o