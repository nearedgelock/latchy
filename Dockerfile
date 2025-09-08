FROM alpine:edge AS builder

RUN echo "#!/bin/ash" > /tmp/entrypoint.sh \
  && echo "cd /Source" >> /tmp/entrypoint.sh \
  && echo "sudo chown developer:developer /Source/vcpkg" >> /tmp/entrypoint.sh \
  && echo "su developer" >> /tmp/entrypoint.sh \
  && echo "make bootstrap" >> /tmp/entrypoint.sh \
  && echo "make -e USESTATIC=true configure" >> /tmp/entrypoint.sh \
  && echo "make build" >> /tmp/entrypoint.sh \
  && chmod 755 /tmp/entrypoint.sh

FROM alpine:edge

COPY --from=builder /tmp/entrypoint.sh /

# vcpkg needs at least cmake version 3.27.1, which is not yet in regular alpine
RUN apk add --no-cache cmake --repository=http://dl-cdn.alpinelinux.org/alpine/edge/main

# Alpine renamed ninja to ninja-build but did NOT remove ninja, which is too old for newer vcpkg.
RUN apk update && \
    apk add --no-cache \
      linux-headers \
      bash \
      build-base \
      curl \
      git \
      g++ \ 
      make \
      meson \
      ninja-build \
      pkgconfig \
      sudo \
      unzip \
      zip \
      zlib \
      perl \
      libc-dev \
      openssl-dev \
      python3
      
RUN cp /usr/lib/ninja-build/bin/ninja /usr/bin/ninja
#RUN ln -s /usr/lib/ninja-build/bin/ninja /usr/bin/ninja

# You may want to adjust the user UID or get it from your user environment
ARG USERNAME=developer      
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN addgroup --gid $USER_GID $USERNAME \
  && adduser -D -u $USER_UID -G $USERNAME $USERNAME \
  && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
  && chmod 0400 /etc/sudoers.d/$USERNAME
  
USER $USERNAME

CMD [ "/entrypoint.sh" ]

