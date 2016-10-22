Cinder block for Newtek's [NDI](http://newtek.com/ndi) video-over-IP protocol. See it in action [here](https://vimeo.com/187360459) .

You will need to [download](http://pages.newtek.com/NDI-Developers.html) the NDI SDK seperately as currently the license does not permit shipping it with 3rd party software.

Before building the samples you will need to export the `NDI_SDK_PATH` env variable to the directory where you unzipped the NDI SDK.

Currently only CMake project files for OS X and Linux are provided so you will need the current `android_linux` branch from Cinder's official repo to build the samples. For best results build the release version of Cinder once you have checked out the `android_linux` branch.

Once you have Cinder setup clone the block into the blocks folder and  `cd Cinder/blocks/Cinder-NDI/samples/sampleDir/proj/cmake && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j4`

Do not forget to `export NDI_SDK_PATH=/path/to/NDI/folder` before attempting to build a sample.

