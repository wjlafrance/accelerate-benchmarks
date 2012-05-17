# Accelerate your apps with Accelerate

## Stacks and Heaps, Memory with C

It's important to understand the difference between stack and heap memory. Each thread has a stack, which functions can use to store data. Stack data is very easily managed because it's cleaned up whenever you exit your "stack frame", generally the function you're in. This has a disadvantage though, because if you want to persist this data, you need to stick it somewhere safe. Also, you're limited to a few dozen kilobytes (don't depend on a certain value - treat your stack nicely). Using too many stack variables causes a stack overflow.

Heap memory is free storage in RAM. The amount of heap space you can use is determined by many factors, such as architecture (32-bit can only address 4GB natively), operating system (iOS limits you pretty severely -- 20MB is getting unsafe), and other factors. But, it'll be much much more than your few dozen kilobytes of stack memory. This is also when memory management comes into play.

To get some heap space, ask the OS for it. Note that `void *` functions return a pointer to a location in memory, and are unlike `void` functions which don't have a return value.

	void *malloc(size_t size);
		Allocates size bytes of memory

	void *calloc(size_t count, size_t size);
		Allocate count times size

	void *realloc(void *ptr, size_t size);
		malloc's size, copies memory, frees old pointer
		If the new size is the same as the old size, returns the old pointer

For example, to make an array of 8 floats in heap space, you might do:

	float *myArray = malloc(8 * sizeof(float));
	float *myArray = calloc(8, sizeof(float));

As we know from Objective-C, you must release what you own, so give it back to the system:

	free(myArray);

Note that there are no reference counts in C. The first person to free a pointer frees it for everyone. Good luck!

## So, why not just use Objective-C?

Objective-C is a great programming language. It's pretty fast, and it's a strict superset of C, so any valid C code is automatically valid Objective-C. But, if you do things the true Objective-C array using NSArray and NSNumber, you'll be significantly slowed down.

First, stack access is generally faster than heap access. It'll be faster to do `int i = 5;` than `int *i = malloc(sizeof(int)); i = 5; /* .. */ free(i);`. I point this out because Objective-C objects don't live in stack space. Also, each Objective-C object that you allocate actually is a `struct objc_object`, which at minimum refers to a `struct objc_class`. This will take a lot more memory than a a simple `int`, which takes either 32 or 64 bits (an int is typically the computer's <a href="http://en.wikipedia.org/wiki/Word_size">word size</a>). 

Second, NSNumber is a particularly good way to slow down your code. Depending on your compiler, `int i = 5;` will probably take two processor instructions (a `mov` and a `push`). Using `[myNumber intValue]` to get an `int` out of an `NSNumber` will be rewritten as the C call, `objc_msgSend(myNumber, @selector(intValue));`, which will then grab the `objc_object`'s `isa`, find the function for the method implementation, etc. In it's defense, `objc_msgSend` is ridiculously optimized and fast, but it's not going to beat C, ever.

In my benchmarks, interacting with `NSNumber` and `NSArray` in place of a `float *` can be up to 6x as slow. Of course, when you're doing things that aren't split-second intensive, that's probably fine, but if you're doing things that limit you to 16ms (time to render a single frame at 60 frames per second), or 22 nanoseconds (an audio frame at 44.1 kHz), you're not going to sit around waiting for Objective-C.

## I thought you said you'd talk about Accelerate

You're still reading! Good!

<a href="http://developer.apple.com/library/mac/#documentation/Accelerate/Reference/AccelerateFWRef/_index.html">Accelerate</a> is an amalgamation of a bunch of different libraries. These include:

* BLAS (Basic Linear Algebra Subprograms)
* ATLAS (Automatically Tuned Linear Algebra Software)
* LAPACK (Linear Algebra PACKage)
* vImage (Image Manipulation)
* vDSP (Digital Signal Processing)

The individual frameworks you'll want to use depend on what you're trying to do. A good general-purpose framework is BLAS, which can be used to drastically speed up operations across an array (the documentation says "vector") of floats or doubles. The best way to demonstrate BLAS is by example:

### Vector Population
For example, say you wanted to create an array of 300 floats, all set to 10.0, in heap space. With standard C, you could do:

	float *myArray = calloc(300, sizeof(float));
	for (int i = 0; i < 300; i++) {
		myArray[i] = 10.0;
	}
	printf("populated the array, freeing it now\n");
	free(myArray);

Using BLAS to parallelize this would be written as:

	float *myArray = calloc(300, sizeof(float));
	catlas_sset(300, 10.0, myArray, 1);
	printf("populated the array, freeing it now\n");
	free(myArray);

### Vector Scaling / Swapping
A situation I ran into recently was reading vertex data from World of WarCraft game data files. Each vertex was stored in the file as X, Z, -Y. I was `memcpy`'ing them onto a `struct Vertex3f`, so they were in my program in the wrong order. OpenGL was expecting these to be in X, Y, Z ordering, so I could have corrected it with a standard C loop:

	for (int i = 0; i < nVertices; i++) {
		float temp = vertices.z * 1; // is actually -Y
		vertices.z = vertices.y; // is actually Z
		vertices.y = temp; // was holding +Y
	}

Instead, I took advantage of BLAS to scale and swap the elements.

	cblas_sscal(nVertices, -1, &(vertices[0].z), 3); // The reference operator returns the pointer to the first Z element in my struct array (actually -Y at this point). By using a stride of 3, it'll only operate on every 3rd float.
	cblas_sswap(nVertices, &(vertices[0].y), 3, &(vertices[0].z), 3); // The first argument strides past the second argument, so interlaced arrays can be swapped

## Conclusion
In my benchmarks, using BLAS instead of looping can give you an 85% to 95% performance improvement on Mac, and 50%+ on iOS. These types of performance improvements are generally unheard of, and since Accelerate is built into iOS 4 and higher, as well as as far back as Mac OS 10.0, you should really consider exploring the possibilities.

Check out <a href="https://github.com/wjlafrance/accelerate-benchmarks/blob/master/accelerate.c">some examples</a> and see for yourself!

