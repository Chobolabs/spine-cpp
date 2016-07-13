# spine-cpp

The spine-cpp runtime provides functionality to load and manipulate skeletal animation from [Spine by Esoteric Software](http://esotericsoftware.com).

Spine-cpp has been developed by [Chobolabs](http://www.chobolabs.com/) for our upcoming title [Mayhem](http://playmayhem.com/)

Spine-cpp uses C++11.

## Usage

### Migrating from spine-c

Please read the [migration guide](https://github.com/Chobolabs/spine-cpp/blob/master/MigrationGuide.md).

### Integrating spine-cpp into a new project

*TBD*

## Limitations

spine-cpp works with data exported from Spine 3.2.07 (but not linked meshes yet). Support for 3.3 is in progress.

Like the spine-c runtime binary loading is not yet supported.

There may be bugs not present in spine-c (mainly in code paths not utilized by our software). Issue reports or pull requests for such would be welcome.

No custom allocation options (yet), except for animation state track entries.

## Differences from spine-c

* Written in C++11 with virtual functions instead of spine-c's C-style polymorphism (Unfortunately it also means slower compile-time)
* Uses a fork of [sajson](https://github.com/chadaustin/sajson) to load json files which provides much faster load time. The time spent in json parsing is about a half of spine-c's.
* Utilizes more cache-friendly data structures which improve performance of the animation state update and world vertex generation. The observed gain is 2-3% on desktop cpus and 10-40% on common arm cpus found on most hand-held devices (due to their lower L1 and L2 cache miss tolerance)
* No support for linked meshes yet, but it will be available soon

You can also see the upcoming features in the [roadmap](https://github.com/Chobolabs/spine-cpp/blob/master/Roadmap.md).

## Other Q&A:

**Should I migrate from spine-c to spine-cpp?**

If you don't have custom spine-c extensions, the migration should be a really easy process. It can literally take minutes to complete. So our advice is for you to test it and see whether you get a noticeable performance improvement.

Generally if your software targets a mobile platform, it would be a good idea to do so. Even if you're satisfied with the performance of your software using spine-c and you're not looking to improve it, spine-cpp uses less cpu and thus less battery power.

If your software loads tens or hundreds of Spine animations you will also see improvements in load time.

**How stable is spine-cpp?**

We use the library in production, but we don't use all of its features (for example we don't use Events of FFD timelines).

Unfortunately we can't afford to test all of the features extensively. If you choose to use spine-cpp and find a bug, we'll do our best to fix it as fast as we can. We hope that we'll be able to fix any bug in spine-cpp, which is not present in spine-c, within 24 hours of learning about it.

**Will you support C++98?**

Unfortunately we don't have such plans.

**Why do you have so many public members instead of setters and getters?**

We wanted to make the migration from spine-c as easy as possible. For example we (and probably a lot of other users) modify some of the values in Spine's data structures, and after a migration the code for this will remain pretty much the same.

Plus it's much more convenient to write `bone.rotation += x;` instead of `bone.setRotation(bone.getRotation() + x);`

**Why are some types `class` and some `struct`?**

The idea behind the differentiation was that types that can be constructed on their own, without depending on other types of the library are `struct`. Those are mainly the `data` types.

We too are not entirely satisfied with the current differentiation and it is subject to change.

**Why did you choose the include path `spinecpp` instead of just `spine`?**

This is mainly for our own convenience because we often run spine-c and spine-cpp side by side to check for the consistency of our implementation and the differences in performance. Since both libraries have the same include file names (like, say, `Animation.h`) we need to differentiate between them somehow.

## Licensing

This is a derivative work of the official spine-c library by Esoteric Software and, as such, is subject to the Spine Runtimes Software License. See [LICENSE.txt](https://github.com/Chobolabs/spine-cpp/blob/master/LICENSE.txt) for details

This Spine Runtime may only be used for personal or internal use, typically to evaluate Spine before purchasing. If you would like to incorporate a Spine Runtime into your applications, distribute software containing a Spine Runtime, or modify a Spine Runtime, then you will need a valid [Spine license](https://esotericsoftware.com/spine-purchase).

The Spine Runtimes are developed with the intent to be used with data exported from Spine. By purchasing Spine, `Section 2` of the [Spine Software License](https://esotericsoftware.com/files/license.txt) grants the right to create and distribute derivative works of the Spine Runtimes.