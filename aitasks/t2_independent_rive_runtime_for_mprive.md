# Motivation
the mprive module is a kotlin multiplatform port of rive android.
this project still contains the original code of rive android. Currently both mprive and rive android in this project depends on the same git submodule that like to the rive-runtime native code.

We would to keep mprive and rive-android features and api as close as possible, but still, it is possible there will be difference between the two at least temporary.

Therefore there is a need to define the dependency of the specific version of rive-runtime for mprive and rive-android separately. In other words we need to introduce an additional git submodule to the project that is the same rive-runtime but referencing a different git commit.
and change mprive to dependend to this git submodules instead of the current git submodule that is the originally dependency for rive-android