APPNAME = cubenorm

include $(ISISROOT)/make/isismake.tsts

commands:
	crop from=$(INPUT)/isisTruth.cub \
	  to= $(OUTPUT)/cropTemp.cub  \
	  sample = 8 \
	  nsamples = 6 \
	  sinc = 1 \
	  line = 8 \
	  nlines = 6 \
	  linc = 1 > /dev/null; \
	$(APPNAME) from= $(OUTPUT)/cropTemp.cub \
	  to=$(OUTPUT)/cubenormTruth2.cub \
	  normalizer = average \
	  preserve = false > /dev/null; \
	$(RM) $(OUTPUT)/cropTemp.cub;
