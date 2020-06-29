FROM adoptopenjdk/openjdk12:latest

# Working directory

WORKDIR .	

# Installation 

RUN apt-get update  && apt-get install zip -y	
RUN apt-get update  && apt-get install unzip -y
RUN apt-get update && apt-get install tesseract-ocr -y
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
RUN curl -s "https://get.sdkman.io" | bash && \
	source "/root/.sdkman/bin/sdkman-init.sh" && \
	sdk install sbt

COPY OS-docker-TP-Scala/ ./

ENV PATH /root/.sdkman/candidates/sbt:$PATH
ENV PATH /root/.sdkman/candidates/sbt/current/bin:$PATH
ENV PATH /root/.sdkman/candidates/sbt/1.3.10/bin/sbt:$PATH
ENV PATH /root/.sdkman/candidates/sbt/1.3.10/lib/local-preloaded/org/scala-sbt/sbt:$PATH
ENV PATH /root/.sdkman/candidates/sbt/1.3.10/bin/sbt:$PATH

CMD ["sbt", "--error", "run"]

