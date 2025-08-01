# Use pre-built PSP development environment
FROM pspdev/pspdev:latest

# Set PSP development environment variables (should already be set, but just in case)
ENV PSPDEV=/usr/local/pspdev
ENV PATH=$PATH:$PSPDEV/bin
ENV PSPSDK=$PSPDEV/psp/sdk

# Set working directory
WORKDIR /project

# Default command
CMD ["/bin/bash"]