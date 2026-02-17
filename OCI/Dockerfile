FROM debian:13
RUN apt-get -qq update \
	&& apt-get -qq --assume-yes install jekyll ruby-jekyll-paginate-v2 ruby-jekyll-sitemap ruby-jekyll-feed ruby-classifier-reborn \
	&& apt-get -qq clean
EXPOSE 4000/tcp 35729/tcp
ENTRYPOINT ["jekyll"]
CMD ["serve", "--host", "0.0.0.0", "--port", "4000", "--drafts", "--future", "--livereload", "--livereload-port", "35729"]
# podman build -t jekyll OCI
# podman run --rm -ti -p 4000:4000 -p 35729:35729 -v "$PWD:$PWD" -w "$PWD" jekyll
# podman run --rm -d  -p 4000:4000 -p 35729:35729 -v "$PWD:$PWD" -w "$PWD" jekyll
# firefox-esr http://localhost:4000/
