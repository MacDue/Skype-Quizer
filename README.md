# Skype Quizer
### Generate a random pub quiz, control it with your phone, and stream it to skype!

![Quiz Demo](./assets/quiz-demo.gif)
<img src="./assets/phone-demo.png" width="300px"/>

This is a little hack to generate a random pub quiz, and stream it to skype (or another app) by creating a virtual webcam.

The properties of the quiz and navigating pages can be controlled via a webapp.

## Building
The quiz streamer currently only supports Linux and requires [v4l2loopback](https://github.com/umlaeute/v4l2loopback) installed.

```sh
# Note if your fake webcam is not /dev/video0 you'll have to edit that in the code.
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. # or -DCMAKE_BUILD_TYPE=Release
```

## Running

Run
```sh
sudo modprobe v4l2loopback # make sure /dev/video0 exists
./run_quiz.sh
killall quiz # You may need to run this once you're done with the quiz!
```

This will start the quiz on port 5000 and the remote on port 3000.
To access the remote on your phone go to ``http://<your_computers_local_ip>:3000`` on your phone.


## Credits
- Questions scraped from https://quiz-questions.net/
- Some JSON utils from https://github.com/yourWaifu/sleepy-discord
- Some code reused from DueUtil (https://dueutil.tech/)
