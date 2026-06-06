# genetic-pong
A genetic algorithm AI that learns to play a pong-like game. Written in C, uses Raylib for graphics.

100 paddles are spawned, each with their own ball. The paddles begin with random weights and get rewarded for ball hits. Once all paddles are dead, the top 10% of paddles are carried over to the next generation. A random set of parents is chosen from the elite paddles to make a child paddle - child paddles inherit DNA (weights) from both parent paddles at a random splicing point, then a slight mutation is applied to the child. The next generation consists of the top 10% of paddles from the previous generation + 90% of their children. Ball and paddle speed increases after every successful hit.


The up and down arrow keys can be used to increase the game speed.

Generation, paddle weights and top hits are printed for the best paddle per generation. The weights can be used in a legit pong game as an AI opponent.


I managed to get a paddle with 473 hits by the 106th generation. I believe 473 may be a plateau with this code since the ball speed will get so fast it will just pass through the paddle. There is a SUB_STEPS variable which is the number of sub steps that are processed per frame to help mitigate the collision tunneling, I found 6 steps to be a sweet spot. 
