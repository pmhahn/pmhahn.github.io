FROM debian:13
RUN apt-get -qq update \
	&& apt-get -qq --assume-yes install jekyll ruby-jekyll-paginate-v2 ruby-jekyll-sitemap ruby-jekyll-feed ruby-classifier-reborn \
	&& apt-get -qq clean
EXPOSE 4000/tcp
ENTRYPOINT ["jekyll"]
CMD ["s"]
# podman build -t jekyll OCI
# podman run --rm -ti -v "$PWD:$PWD" -w "$PWD" jekyll
