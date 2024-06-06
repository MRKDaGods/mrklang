# mrklang
[INDEV] mrklang, a programming language inspired by game hacking and utility

I have been having many concepts since I was 15 years old about creating my own programming language, I have attempted many trials before for the last 2 years, and I am finally serious about it. I have uploaded the project on github, so everyone would check it out :D The main idea of the language would be writing flawless c++/c/c#/java code at the same time, in one file, on all platforms. I am constantly working on it, although my time does not help because of school and online classes :( Hopefully it would be something to talk about one day in the future :) No one motivated me irl, so I thought about sharing it here

Code example(Vector3.mrk):
```
i math;

c Vector3 {
	v int x {
		r 9 //default value
	}
	v int y
	v int z
	
	m .{
                p {
			int x1
			int y1
			int z1
		}

		x = x1
		y = y1
		z = z1

	}

	m float getMag {

		v float m

		__cs {
			
			m = Math.Sqrt(x^2 + y^2 + z^2);
		}

		__cpp {

			float* mem = (float*)malloc(sizeof(float));
			*mem = m;
			std::cout << "Testing cpp, magPtr=" << mem << '\n';

		}

		$ANDROID
		__java {

			MainActivity.instance().ShowNewMag(m);
		
		}

		r m

	}
}
```
