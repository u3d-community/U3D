# Build skyboxes from HDR
resolution=2048
for skyFile in $1/*.hdr; do
    # Get just the filename
    skyName=$(basename -- $skyFile)

    # And get rid of the extension
    skyName=${skyName%.*}

    # Now render the HDR into 6 cubefaces using blender
    blender --background root.blend --python render-skybox.py -- $skyFile $skyName $resolution $1
done
