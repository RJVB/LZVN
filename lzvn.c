/*
 * Created: 31 October 2014
 * Name...: lzvn.c
 * Author.: Pike R. Alpha
 * Purpose: Command line tool to LZVN encode/decode a prelinked kernel/file.
 *
 * Updates:
 *			- Support for lzvn_decode() added (Pike R. Alpha, November 2014).
 *			- Support for prelinkedkernels added (Pike R. Alpha, December 2014).
 *			- Debug output data added (Pike R. Alpha, July 2015).
 *			- Prelinkedkerel check added (Pike R. Alpha, August 2015).
 *			- Mach header injection for prelinkedkernels added (Pike R. Alpha, August 2015).
 *			- Extract kernel option added (Pike R. Alpha, August 2015).
 *			- Extract dictionary option added (Pike R. Alpha, September 2015).
 *			- Extract kexts option added (Pike R. Alpha, September 2015).
 *			– Function find_load_command() added (Pike R. Alpha, September 2015).
 *			- Save Dictionary.plist in proper XML format.
 *			- Show number of signed and unsigned kexts.
 *			- Usage now shows 'lzvn' once.
 *			- Show list of kexts added (Pike R. Alpha, Januari 2016).
 *			- Fixed encoding of files with a FAT header (Pike R. Alpha, July 2017).
 */

#include "lzvn.h"
#ifdef HAS_FULLVERSION
#   include "lzvn_fullversion.h"
#endif
#include <errno.h>

//==============================================================================

int main(int argc, const char * argv[])
{
	FILE *fp								= NULL;
	FILE *ofp								= NULL;

	unsigned char * fileBuffer				= NULL;
	unsigned char * workSpaceBuffer			= NULL;
	unsigned char * bufend					= NULL;
	unsigned char * buffer					= NULL;

	PrelinkedKernelHeader * prelinkHeader	= NULL;
	struct fat_header * fatHeader			= NULL;
	struct fat_arch   * fatArch				= NULL;
	
	unsigned int offset						= 0;
	
	unsigned long fileLength				= 0;
	unsigned long byteshandled				= 0;
#ifdef __APPLE__
	unsigned long file_adler32				= 0;
#endif

	size_t compressedSize					= 0;
	size_t workSpaceSize					= 0;

	if (argc < 3 || argc > 4)
	{
#ifdef LZVN_FULL_VERSION_STRING
        printf("lzvn version %s\n", LZVN_FULL_VERSION_STRING);
#endif
		printf("Usage (encode): lzvn <infile> <outfile>\n");
		printf("Usage (decode): lzvn -d <infile> <outfile>\n");
#ifdef __APPLE__
		printf("Usage (decode): lzvn -d <path/prelinkedkernel> kernel\n");
		printf("Usage (decode): lzvn -d <path/prelinkedkernel> dictionary\n");
		printf("Usage (decode): lzvn -d <path/prelinkedkernel> kexts\n");
		printf("Usage (decode): lzvn -d <path/prelinkedkernel> list\n");
#endif
		exit(-1);
	}
	else
	{
		// Do we need to decode a file?
		if (!strncmp(argv[1], "-d", 2))
		{
			// Yes. Try to open the file.
			fp = fopen(argv[2], "rb");

			// Check file pointer.
			if (fp == NULL)
			{
				printf("Error: Opening of %s failed (%s)... exiting\nDone.\n", argv[1], strerror(errno));
				exit(-1);
			}
			else
			{
				fseek(fp, 0, SEEK_END);
				fileLength = ftell(fp);
				
				printf("Filesize: %ld bytes\n", fileLength);
				
				fseek(fp, 0, SEEK_SET);
				fileBuffer = malloc(fileLength);
				
				if (fileBuffer == NULL)
				{
					printf("ERROR: Failed to allocate file buffer... exiting\nAborted!\n\n");
					fclose(fp);
					exit(-1);
				}
				else
				{
					fread(fileBuffer, fileLength, 1, fp);
					fclose(fp);

#ifdef __APPLE__
					// Check for a FAT header.
					fatHeader = (struct fat_header *) fileBuffer;

					if (fatHeader->magic == FAT_CIGAM)
					{
						unsigned int i = 1;
						fatArch = (struct fat_arch *)(fileBuffer + sizeof(fatHeader));
						prelinkHeader = (PrelinkedKernelHeader *)((unsigned char *)fileBuffer + OSSwapInt32(fatArch->offset));

						while ((i < OSSwapInt32(fatHeader->nfat_arch)) && (prelinkHeader->signature != OSSwapInt32('comp')))
						{
							printf("Scanning ...\n");
							fatArch = (struct fat_arch *)((unsigned char *)fatArch + sizeof(fatArch));
							prelinkHeader = (PrelinkedKernelHeader *)(fileBuffer + OSSwapInt32(fatArch->offset));
							++i;
						}

						// Is this a LZVN compressed file?
						if (prelinkHeader->compressType == OSSwapInt32('lzvn'))
						{
							printf("Prelinkedkernel found!\n");
							fileBuffer = (unsigned char *)prelinkHeader + sizeof(PrelinkedKernelHeader);
							workSpaceSize = OSSwapInt32(prelinkHeader->uncompressedSize);
							fileLength = OSSwapInt32(prelinkHeader->compressedSize);
						}
						else
						{
							printf("ERROR: Unsupported compression format detected!\n");
							exit(-1);
						}
					}
					else
#endif
					{
#ifdef __APPLE__
						prelinkHeader = (PrelinkedKernelHeader *)(unsigned char *)fileBuffer;
						
						if (prelinkHeader->signature == OSSwapInt32('comp') && prelinkHeader->compressType == OSSwapInt32('lzvn'))
						{
							fileBuffer = (unsigned char *)prelinkHeader + sizeof(PrelinkedKernelHeader);
							workSpaceSize = OSSwapInt32(prelinkHeader->uncompressedSize);
							fileLength = OSSwapInt32(prelinkHeader->compressedSize);
						}
						else
#endif
						{
							// printf("Getting WorkSpaceSize\n");
							workSpaceSize = lzvn_encode_work_size();
						}
					}

					// printf("workSpaceSize: %ld \n", workSpaceSize);
					workSpaceBuffer = malloc(workSpaceSize);

					if (workSpaceBuffer == NULL)
					{
						printf("ERROR: Failed to allocate workSpaceBuffer ... exiting\nAborted!\n\n");
						exit(-1);
					}
					else
					{
						/* if (workSpaceSize > 0x80000)
						{ */
							compressedSize = lzvn_decode(workSpaceBuffer, workSpaceSize, fileBuffer, fileLength);

							if (compressedSize > 0)
							{
#ifdef __APPLE__
								// Are we unpacking a prelinkerkernel?
								if (is_prelinkedkernel(workSpaceBuffer))
								{
									printf("Checking adler32 ... ");

									// Yes. Check adler32.
									if (OSSwapInt32(prelinkHeader->adler32) != local_adler32(workSpaceBuffer, workSpaceSize))
									{
										printf("ERROR: Adler32 mismatch!\n");
										free(workSpaceBuffer);
										fclose(fp);
										exit(-1);
									}
									else
									{
										printf("OK (0x%08x)\n", OSSwapInt32(prelinkHeader->adler32));

										// Do we need to write the _PrelinkInfoDictionary to disk?
										if (strstr(argv[3], "dictionary") || strstr(argv[3], "kexts"))
										{
											printf("Extracting dictionary ...\n");
											saveDictionary(workSpaceBuffer);
										}

										if (strstr(argv[3], "kexts"))
										{
											printf("Extracting kexts ...\n");
											saveKexts(workSpaceBuffer);
										}
										
										if (strstr(argv[3], "list"))
										{
											printf("Getting list of kexts ...\n");
											listKexts(workSpaceBuffer);
										}
										
										// Do we need to write the kernel to disk?
										if (strcmp(argv[3], "kernel") == 0)
										{
											printf("Extracting kernel ...\n");
											saveKernel(workSpaceBuffer);
										}

										if (gSaveAll)
										{
											openFile((char *)argv[3]);
											printf("Decoding prelinkedkernel ...\nWriting data to: %s\n", argv[3]);
											fwrite(workSpaceBuffer, 1, compressedSize, fp);
											printf("%ld bytes written\n", ftell(fp));
											fclose(fp);
										}
									}
								}
								else
#endif
								{
									printf("Decoding data ...\nWriting data to: %s\n", argv[3]);
									fwrite(workSpaceBuffer, 1, compressedSize, fp);
									printf("%ld bytes written\n", ftell(fp));
									fclose(fp);
								}
							}
						/* }
						else
						{
							while ((compressedSize = lzvn_decode(workSpaceBuffer, workSpaceSize, fileBuffer, fileLength)) > 0)
							{
								printf("compressedSize: %ld\n", compressedSize);
								fwrite(workSpaceBuffer, 1, compressedSize, fp);
								fileBuffer += workSpaceSize;
								byteshandled += workSpaceSize;
							}
						
							fileBuffer -= byteshandled;
						} */

						free(workSpaceBuffer);

						printf("\nDone.\n");
						exit(0);
					}
				}
			}
		}
		else // Do we need to encode a file?
		{
			fp = fopen(argv[1], "rb");
            ofp = fopen (argv[2], "wb");

			if (fp == NULL)
			{
				printf("Error: Opening of %s failed (%s)... exiting\nDone.\n", argv[1], strerror(errno));
				exit(-1);
			}
			else if (ofp == NULL)
            {
                fprintf(stderr, "Error opening output file '%s': %s\n", argv[2], strerror(errno));
                exit(-1);
            }
			else
			{
				fseek(fp, 0, SEEK_END);
				fileLength = ftell(fp);
				printf("fileLength...: %ld/0x%08lx - %s\n", fileLength, fileLength, argv[1]);
				fseek(fp, 0, SEEK_SET);
                if (fileLength < LZVN_MINIMUM_COMPRESSABLE_SIZE)
                {
                    printf("WARNING: files under %d bytes are ignored\n", LZVN_MINIMUM_COMPRESSABLE_SIZE);
                    fclose(fp);
                    exit(0);
                }

				fileBuffer = malloc(fileLength);

				if (fileBuffer == NULL)
				{
					printf("ERROR: Failed to allocate file buffer... exiting\nAborted!\n\n");
					fclose(fp);
					exit(-1);
				}
				else
				{
					fread(fileBuffer, fileLength, 1, fp);
					fclose(fp);

					size_t workSpaceSize = lzvn_encode_work_size();
					void * workSpace = malloc(workSpaceSize);

					if (workSpace == NULL)
					{
						printf("\nERROR: Failed to allocate workspace... exiting\nAborted!\n\n");
						exit(-1);
					}
					else
					{
						printf("workSpaceSize: %ld/0x%08lx\n", workSpaceSize, workSpaceSize);

						if (fileLength > workSpaceSize)
						{
							workSpaceSize = fileLength;
						}

						workSpaceBuffer = (void *)malloc(workSpaceSize);

						if (workSpaceBuffer == NULL)
						{
							printf("ERROR: Failed to allocate workSpaceBuffer ... exiting\nAborted!\n\n");
							exit(-1);
						}
						else
						{
#ifdef __APPLE__
							// Check for a FAT header.
							fatHeader = (struct fat_header *) fileBuffer;
							
							if (fatHeader->magic == FAT_CIGAM)
							{
								fatArch = (struct fat_arch *)(fileBuffer + sizeof(fatHeader));
								offset = OSSwapInt32(fatArch->offset);
                                printf("fat header detected, offset=%u, nstructs=%d\n",
                                       offset, OSSwapInt32(fatHeader->nfat_arch));
							
                                file_adler32 = local_adler32((fileBuffer + offset), fileLength);
                                printf("adler32......: 0x%08lx\n", file_adler32);
							}
#endif


							size_t outSize = lzvn_encode(workSpaceBuffer, workSpaceSize, (u_int8_t *)(fileBuffer + offset), (size_t)fileLength, workSpace);
							printf("outSize......: %ld/0x%08lx\n", outSize, outSize);

							free(workSpace);

							if (outSize != 0)
							{
								bufend = workSpaceBuffer + outSize;
								compressedSize = bufend - workSpaceBuffer;
								printf("compressedSize.....: %ld/0x%08lx\n", compressedSize, compressedSize);

#ifdef __APPLE__
                                // Do we need to inject the mach header?
                                if (fatHeader->magic == FAT_CIGAM && is_prelinkedkernel(fileBuffer + offset))
                                {
                                    printf("Fixing file header for prelinkedkernel ...\n");

                                    // Inject arch offset into the header.
                                    gFileHeader[5] = OSSwapInt32(sizeof(gFileHeader) + outSize - 28);
                                    // Inject the value of file_adler32 into the header.
                                    gFileHeader[9] = OSSwapInt32(file_adler32);
                                    // Inject the uncompressed size into the header.
                                    gFileHeader[10] = OSSwapInt32(fileLength);
                                    // Inject the compressed size into the header.
                                    gFileHeader[11] = OSSwapInt32(compressedSize);

                                    printf("Writing fixed up file header ...\n");

                                    fwrite(gFileHeader, (sizeof(gFileHeader) / sizeof(u_int32_t)), 4, ofp);
                                }
#endif

                                printf("Writing workspace buffer ...\n");

                                fwrite(workSpaceBuffer, outSize, 1, ofp);
                                fclose(ofp);
                                ofp = NULL;
                                printf("Done.\n");
							}
						}

						free(fileBuffer);
						free(workSpaceBuffer);

						exit(0);

					}
				}
			}
		}
	}

	if (ofp)
    {
        // a compression operation was aborted; cleanup
        fclose(ofp);
        unlink(argv[2]);
    }

	exit(-1);
}
