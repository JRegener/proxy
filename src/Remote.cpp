#include "Remote.h"

namespace proxy {

	void Remote::sendAsyncRequest (Ref<Request> request) {
		LOG_FUNCTION_DEBUG;

		if (socket.is_open ()) {
			beast::http::async_write (socket, *request,
									  asio::bind_executor (
										  strand,
										  std::bind (&Remote::handleWrite,
													 shared_from_this (),
													 request,
													 std::placeholders::_1,
													 std::placeholders::_2)
									  ));
		}
		else {
			resolver.async_resolve (address, std::to_string (port),
									asio::bind_executor (
										strand,
										std::bind (&Remote::handleResolve,
												   shared_from_this (),
												   request,
												   std::placeholders::_1,
												   std::placeholders::_2)
									));
		}
	}

	void Remote::prepareChunk (size_t size)
	{
		ch.reserve (static_cast<std::size_t>(size));
		ch.clear ();
	}

	void Remote::accumulateChunk (beast::string_view body)
	{
	}

	void Remote::handleResolve (Ref<Request> request,
								boost::system::error_code ec,
								tcp::resolver::results_type result) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		asio::async_connect (socket, result.begin (), result.end (),
							 asio::bind_executor (
								 strand,
								 std::bind (&Remote::handleConnect,
											shared_from_this (),
											request,
											std::placeholders::_1)));
	}

	void Remote::handleConnect (Ref<Request> request,
								const boost::system::error_code& ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		sendAsyncRequest (request);
	}

	void Remote::handleWrite (Ref<Request> request,
							  boost::system::error_code ec,
							  std::size_t bytes_tranferred) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}


		// в начале читаем заголовок если он chunked
		// то читаем тело запроса одним способом
		// если простой запрос просто пересылаем
		// если sse TODO

		Ref<ResponseHeaderParser> header = createRef<ResponseHeaderParser> ();
		header->header_limit (std::numeric_limits<std::uint32_t>::max ());

		beast::http::async_read_header (socket, buffer, *header,
										asio::bind_executor (
											strand,
											std::bind (&Remote::handleReadHeader,
													   shared_from_this (),
													   header,
													   std::placeholders::_1)
										));
	}



	void Remote::handleReadHeader (Ref<ResponseHeaderParser> header,
								   boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		Ref<ResponseHeaderParser> respHeader = createRef<ResponseHeaderParser> (header->get ());
		
		if (header->chunked ()) {
			std::cout << "chunked response" << std::endl;

			// а вообще имеет смысл ждать подтверждения того, что header был отправлен?
			client->sendHeaderResponseAsync (respHeader);

			readChunk (respHeader);

		}
		else {
			std::cout << "normal response" << std::endl;
			Ref<ResponseParser> response = createRef<ResponseParser> (std::move (*header));
			response->body_limit (std::numeric_limits<std::uint64_t>::max ());

			// TODO:
			// headerhandler + bodyHandler
			// or full response handler
			// responseHandler
			// 
			// receive remote http response

			// read full body
			beast::http::async_read (socket, buffer, *response,
									 asio::bind_executor (
										 strand,
										 std::bind (&Remote::handleReadBody,
													shared_from_this (),
													response,
													std::placeholders::_1)
									 ));
		}
	}

	void Remote::handleReadBody (Ref<ResponseParser> response, boost::system::error_code ec) {
		LOG_FUNCTION_DEBUG;

		if (ec) {
			logBoostError (ec);
			return;
		}

		// send back to client
		client->sendResponseAsync (createRef<Response> (response->get ()));
	}



	void Remote::readChunk (Ref<ResponseHeaderParser> header) {
		LOG_FUNCTION_DEBUG;
		std::string cch;
		Ref<std::string> chunk = createRef<std::string> ();

		header->on_chunk_header ([](std::uint64_t size, boost::string_view extensions,
									boost::system::error_code& ec) {
										LOG_FUNCTION_DEBUG;

										//ce.parse (extensions, ev);
										if (ec)
											return;

										// See if the chunk is too big
										if (size > (std::numeric_limits<std::size_t>::max)()) {
											ec = beast::http::error::body_limit;
											return;
										}
										//self->prepareChunk (size);
										// Make sure we have enough storage, and
										// reset the container for the upcoming chunk
										//chunk->reserve (static_cast<std::size_t>(size));
										//chunk->clear ();

										//cch.reserve (static_cast<std::size_t>(size));
										//cch.clear ();
								 });

		header->on_chunk_body ([](std::uint64_t remain, boost::string_view body,
								  boost::system::error_code& ec) {
									  LOG_FUNCTION_DEBUG;
									  // If this is the last piece of the chunk body,
									  // set the error so that the call to `read` returns
									  // and we can process the chunk.
									  if (remain == body.size ())
										  ec = beast::http::error::end_of_chunk;

									  // Append this piece to our container
									  //chunk->append (body.data (), body.size ());

									  // The return value informs the parser of how much of the body we
									  // consumed. We will indicate that we consumed everything passed in.
									  return body.size ();
							   });

		//beast::http::async_read (socket, buffer, *header,
		//						 asio::bind_executor (
		//							 strand,
		//							 std::bind (&Remote::handleReadChunk,
		//										shared_from_this (),
		//										header,
		//										chunk,
		//										std::placeholders::_1,
		//										std::placeholders::_2)
		//						 ));


		while (!header->is_done ())
		{
			std::cout << "while" << std::endl;
			// Read as much as we can. When we reach the end of the chunk, the chunk
			// body callback will make the read return with the end_of_chunk error.
			boost::system::error_code ec;
			beast::http::read (socket, buffer, *header, ec);
			if (!ec)
				continue;
			else if (ec != beast::http::error::end_of_chunk)
				return;
			else
				ec = {};
		}
		int a = 0;
	}

	void Remote::handleReadChunk (Ref<ResponseHeaderParser> header, Ref<std::string> chunk,
								  boost::system::error_code ec, std::size_t bytes_tranferred) {
		LOG_FUNCTION_DEBUG;

		if (!header->is_done ()) {
			if (ec != beast::http::error::end_of_chunk) {
				std::cout << "errrr" << std::endl;
				return;
			}
			else {
				logBoostError (ec);
			}

			// send chunk to client
			client->sendChunkAsync (*chunk);

			readChunk (header);
			return;
		}

		client->sendChunkLastAsync ();
	}

	void Remote::onChunkHeader (std::uint64_t size, boost::string_view extensions,
								boost::system::error_code& ec) {
		LOG_FUNCTION_DEBUG;

		//ce.parse (extensions, ev);
		if (ec)
			return;

		// See if the chunk is too big
		if (size > (std::numeric_limits<std::size_t>::max)()) {
			ec = beast::http::error::body_limit;
			return;
		}

		// Make sure we have enough storage, and
		// reset the container for the upcoming chunk
		//chunk.reserve (static_cast<std::size_t>(size));
		//chunk.clear ();
	}

	size_t Remote::onChunkBody (std::uint64_t remain, boost::string_view body,
								boost::system::error_code& ec) {
		LOG_FUNCTION_DEBUG;
		// If this is the last piece of the chunk body,
		// set the error so that the call to `read` returns
		// and we can process the chunk.
		if (remain == body.size ())
			ec = beast::http::error::end_of_chunk;

		//std::cout << body << std::endl;
		// TODO: to storage 
		//client.sendChunkAsync (ec, body);

		//if (ec) client.sendChunkAsync (remain, body, true);

		// Append this piece to our container
		//chunk.append (body.data (), body.size ());

		// The return value informs the parser of how much of the body we
		// consumed. We will indicate that we consumed everything passed in.
		return body.size ();
	}


}