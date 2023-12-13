CFLAGS?=-Wall -std=c90

all: phy_zip phy_unzip

phy_zip: phyzip.c ../src/lz77.c
	@$(CC) -o phy_zip $(CFLAGS) -I../include phyzip.c ../src/lz77.c

phy_unzip: phyunzip.c ../src/lz77.c
	@$(CC) -o phy_unzip $(CFLAGS) -I../include phyunzip.c ../src/lz77.c

clean :
	@$(RM) phy_zip phy_unzip *.o
