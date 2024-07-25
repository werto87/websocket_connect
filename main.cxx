
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
boost::asio::awaitable<void>
sendSomething (boost::asio::ip::tcp::endpoint endpoint)
{
  auto ws = boost::beast::websocket::stream<boost::beast::tcp_stream> (co_await boost::beast::net::this_coro::executor);
  // Set a timeout on the operation
  boost::beast::get_lowest_layer (ws).expires_after (std::chrono::seconds (30));

  // Make the connection on the IP address we get from a lookup
  co_await boost::beast::get_lowest_layer (ws).async_connect (endpoint, boost::asio::use_awaitable);
  co_await ws.async_handshake (endpoint.address ().to_string () + std::to_string (endpoint.port ()), "/", boost::asio::use_awaitable);
  auto text = std::string ("abc");
  auto message = boost::beast::net::buffer (text);
  co_await ws.async_write (message, boost::asio::use_awaitable);
  boost::beast::flat_buffer buffer{};

  // Read a message into our buffer
  co_await ws.async_read (buffer, boost::asio::use_awaitable);

  // Close the WebSocket connection
  co_await ws.async_close (boost::beast::websocket::close_code::normal, boost::asio::use_awaitable);

  // If we get here then the connection is closed gracefully

  // The make_printable() function helps print a ConstBufferSequence
  std::cout << boost::beast::make_printable (buffer.data ()) << std::endl;
}

int
main ()
{
  auto context = boost::asio::io_context{};
  auto gameEndpoint = boost::asio::ip::tcp::endpoint{ boost::asio::ip::tcp::v4 (), 4242 };
  boost::asio::co_spawn (context, sendSomething (gameEndpoint),
                         // If the awaitable exists with an exception, it gets delivered here as `e`.
                         // This can happen for regular errors, such as connection drops.
                         [] (std::exception_ptr e) {
                           if (e) try
                               {
                                 std::rethrow_exception (e);
                               }
                             catch (std::exception &ex)
                               {
                                 std::cerr << "Error: " << ex.what () << "\n";
                               }
                         });
  context.run ();
  return 0;
}