# XShipWars 移植用 Dev Container
# ベース: Anthropic の Claude Code 参照 devcontainer (anthropics/claude-code/.devcontainer)
# を元に、Debian trixie (gcc 14) と X11 開発・ヘッドレス実行環境を追加したもの。
FROM node:22-trixie

ARG TZ
ENV TZ="$TZ"

ARG CLAUDE_CODE_VERSION=latest

# 基本ツール + ファイアウォール用ツール
RUN apt-get update && apt-get install -y --no-install-recommends \
  less git procps sudo fzf man-db unzip gnupg2 gh jq nano vim curl ca-certificates \
  iptables ipset iproute2 dnsutils aggregate \
  && apt-get clean && rm -rf /var/lib/apt/lists/*

# XShipWars のビルド・デバッグ・ヘッドレス実行に必要なもの
# (ファイアウォール起動後は apt が使えないので、追加パッケージはここに書いてリビルドする)
RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential gdb valgrind strace ltrace bear \
  libx11-dev libxext-dev libxpm-dev libxt-dev libxinerama-dev x11proto-dev \
  xvfb xauth x11-apps x11-utils xdotool imagemagick \
  xfonts-base \
  && apt-get clean && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/local/share/npm-global && chown -R node:node /usr/local/share

RUN mkdir /commandhistory \
  && touch /commandhistory/.bash_history \
  && chown -R node /commandhistory

ENV DEVCONTAINER=true

RUN mkdir -p /workspace /home/node/.claude && \
  chown -R node:node /workspace /home/node/.claude

WORKDIR /workspace

USER node

ENV NPM_CONFIG_PREFIX=/usr/local/share/npm-global
ENV PATH=$PATH:/usr/local/share/npm-global/bin
ENV SHELL=/bin/bash
ENV EDITOR=nano
ENV VISUAL=nano
RUN echo "export PROMPT_COMMAND='history -a' && export HISTFILE=/commandhistory/.bash_history" >> /home/node/.bashrc

RUN npm install -g @anthropic-ai/claude-code@${CLAUDE_CODE_VERSION}

COPY init-firewall.sh /usr/local/bin/
USER root
RUN chmod +x /usr/local/bin/init-firewall.sh && \
  echo "node ALL=(root) NOPASSWD: /usr/local/bin/init-firewall.sh" > /etc/sudoers.d/node-firewall && \
  chmod 0440 /etc/sudoers.d/node-firewall
USER node
