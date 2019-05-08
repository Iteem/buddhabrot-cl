#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <CL/cl.hpp>

#include <SFML/Graphics.hpp>

double clamp( double x )
{
	return x < 0 ? 0 : x > 1 ? 1 : x;
}

//const sf::Vector2i screen_size(1920 * 4, 1080 * 4);
//const sf::Vector2i window_size(1920, 1080);
//const sf::Vector2i screen_size(1920 * 2, 1080 * 2);
//const sf::Vector2i window_size(1920, 1080);
const sf::Vector2i screen_size(1024 * 8, 1024 * 8);
//const sf::Vector2i screen_size(1024 * 8, 1024 * 8);
const sf::Vector2i window_size(1024 * 2, 1024 * 2);

const float fullRStart = -2.2f;
const float fullExtent = 3.f;

const double gamma = 2.;
const unsigned int tile_size = 512;
const unsigned int update_frequency = 200;

inline void checkErr(cl_int err, const char * name)
{
	if ( err != CL_SUCCESS ){
		std::cerr << "ERROR: " << name << " (" << err << ")" << std::endl;
		exit( EXIT_FAILURE );
	}
}

int main()
{
	cl_int err;

	// Get available platforms.
	std::vector< cl::Platform > platforms;
	cl::Platform::get( &platforms );

	checkErr( platforms.size() != 0 ? CL_SUCCESS : -1, "cl::Platform::get" );
	std::cerr << "Platform number is: " << platforms.size() << std::endl;

	std::string platformVendor;
	for( int i = 0; i < platforms.size(); ++i ) {
		platforms[i].getInfo(( cl_platform_info ) CL_PLATFORM_VENDOR, &platformVendor );
		std::cerr << "Platform is by: " << platformVendor << "\n";
	}

	// Create context.
	cl_context_properties cprops[3] = { CL_CONTEXT_PLATFORM, ( cl_context_properties )( platforms[0] )(), 0 };

	cl::Context context( CL_DEVICE_TYPE_DEFAULT, cprops, NULL, NULL, &err);
	checkErr( err, "Conext::Context()" );


	// Get available devices.
	std::vector<cl::Device> devices;
	devices = context.getInfo<CL_CONTEXT_DEVICES>();
	checkErr( devices.size() > 0 ? CL_SUCCESS : -1, "devices.size() > 0" );

	// Load program.
	std::ifstream file( "kernels/renderer.cl" );
	checkErr( file.is_open() ? CL_SUCCESS:-1, "Failed to load renderer.cl" );
	std::string prog( std::istreambuf_iterator<char>(file), ( std::istreambuf_iterator<char>() ) );

	cl::Program::Sources source( 1, std::make_pair( prog.c_str(), prog.length()+1 ) );
	cl::Program program( context, source );
	err = program.build( devices, "" );
	if( err == CL_BUILD_PROGRAM_FAILURE ){
		std::cout << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>( devices[0] ) << std::endl;
	}
	checkErr(err, "Program::build()");

	// Load image.
	sf::Image img;
	img.create( screen_size.x, screen_size.y, sf::Color::Black );

	// Copy image to our vector.
	std::vector<cl_uint> image( img.getSize().x * img.getSize().y * 3, 0 );
	/*for( unsigned int x = 0; x < img.getSize().x; ++x ){
		for ( unsigned int y = 0; y < img.getSize().y ;++y ){
			sf::Color col( img.getPixel( x, y ) );
			image[x + y * img.getSize().x] = cl_float4{ col.r / 255.f, col.g / 255.f, col.b / 255.f, 0.f };
		}
	}*/

	// Buffer.
	//cl::Buffer clImage( context, CL_MEM_READ_WRITE, image.size() * sizeof( cl_uint4 ), nullptr, &err );
	//cl::Buffer clImage( context, CL_MEM_READ_WRITE, image.size() * sizeof( cl_uint ) / 2 * 3, nullptr, &err );
	cl::Buffer clImage( context, CL_MEM_READ_WRITE, image.size() * sizeof( cl_uint ) * 3, nullptr, &err );
	checkErr(err, "Buffer::Buffer()");

	// Load kernel.
	cl::Kernel kernel( program, "buddhabrot", &err );
	checkErr(err, "Kernel::Kernel()" );
	//cl::Buffer clSeeds( context, CL_MEM_READ_WRITE, screen_size.x * screen_size.y * sizeof( cl_uint ), nullptr, &err );
	//checkErr(err, "Buffer::Buffer()");
	err = kernel.setArg( 0, clImage);
	checkErr( err, "Kernel::setArg()" );
	//err = kernel.setArg( 1, clSeeds );
	//checkErr( err, "Kernel::setArg()" );
	err = kernel.setArg( 1, screen_size.x );
	//err = kernel.setArg( 1, screen_size.x / 2 );
	checkErr( err, "Kernel::setArg()" );
	err = kernel.setArg( 2, screen_size.y );
	checkErr( err, "Kernel::setArg()" );

	// Queue.
	cl::CommandQueue queue(context, devices[0], 0, &err);
	checkErr(err, "CommandQueue::CommandQueue()");

	// Copy data to buffer.
	//err = queue.enqueueWriteBuffer( clImage, CL_TRUE, 0, image.size() * sizeof( cl_uint4 ), image.data() );
	//checkErr( err, "ComamndQueue::enqueueWriteBuffer()" );
	//err = queue.enqueueWriteBuffer( clSpheres, CL_TRUE, 0, spheres.size() * sizeof( Sphere ), spheres.data() );
	//checkErr( err, "ComamndQueue::enqueueWriteBuffer()" );

	/*{
		std::vector<cl_uint> seeds( screen_size.x * screen_size.y );
		std::mt19937 gen( std::time( nullptr ) );
		std::uniform_int_distribution<cl_uint> dis( 1, 2147483647 - 1 );
		for( int i = 0; i < screen_size.x * screen_size.y; ++i ){
			seeds[i] = dis( gen );
		}
		err = queue.enqueueWriteBuffer( clSeeds, CL_TRUE, 0, seeds.size() * sizeof( cl_uint ), seeds.data() );
		checkErr( err, "ComamndQueue::enqueueWriteBuffer()" );
	}*/

	sf::RenderWindow window;
	window.create( sf::VideoMode( window_size.x, window_size.y ), "BuddhabrotCl");

	sf::Texture texture;
	texture.loadFromImage( img );
	texture.setSmooth( true );
	sf::Sprite sprite( texture );
	sprite.setScale( static_cast<float>( window_size.x ) / static_cast<float>( screen_size.x ),
	                 static_cast<float>( window_size.y ) / static_cast<float>( screen_size.y ) );

    unsigned int tile_lines = screen_size.x / tile_size;
    auto additional_depth = static_cast<unsigned  int>( std::log2( tile_lines ) );

	unsigned int samples = 0;
	sf::Clock sampleTime;

    int depth = 2;
    int square = 0;

	while( window.isOpen() ){
		sf::Event event;
		while( window.pollEvent( event ) ){
			if( event.type == sf::Event::Closed ){
				window.close();
			}

			if( event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F2 )
				img.saveToFile( "BuddhabrotCl-" + std::to_string( samples ) + ".png" );
		}

		err = kernel.setArg( 3, samples );
		checkErr( err, "Kernel::setArg()" );

		int lines = 1 << depth;
		float extent = fullExtent / lines;
		float rStart = fullRStart + extent * (square % lines);
		float iStart = -fullExtent / 2.f + extent * (square / lines);

		err = kernel.setArg( 4, rStart );
		checkErr( err, "Kernel::setArg()" );
		err = kernel.setArg( 5, extent );
		checkErr( err, "Kernel::setArg()" );
		err = kernel.setArg( 6, iStart );
		checkErr( err, "Kernel::setArg()" );
		err = kernel.setArg( 7, extent );
		checkErr( err, "Kernel::setArg()" );

		// Do the thing.
		cl::Event clEvent;
		err = queue.enqueueNDRangeKernel( kernel, cl::NullRange, cl::NDRange( img.getSize().x, img.getSize().y ), cl::NDRange(8,8), nullptr, &clEvent );
		checkErr(err, "ComamndQueue::enqueueNDRangeKernel()");

		samples += 1;

		err = queue.finish();
		checkErr( err, "queue.finish()" );


		window.setTitle( "BuddhabrotCl - " +  std::to_string( samples ) +  "SPP, " + std::to_string( static_cast<int>( 8 * screen_size.x * screen_size.y / sampleTime.restart().asSeconds() / 1e6 ) ) + "M SPS" );

		// Only fetch the image every so often.
		if( samples % update_frequency == 0 ){
			// Read buffer back into the vector.
			//err = queue.enqueueReadBuffer( clImage, CL_TRUE, 0, image.size() * sizeof( cl_uint4 ), image.data() );
			err = queue.enqueueReadBuffer( clImage, CL_TRUE, 0, image.size() * sizeof( cl_uint ), image.data() );
			checkErr( err, "ComamndQueue::enqueueReadBuffer()" );

			// Copy vector back into the image.
            int numSquares = lines * lines;
			double factor = static_cast<float>(numSquares) / (80.f * samples);
			//double factor = 1. / (80 * samples);
			//for( unsigned int x = 0; x < img.getSize().x / 2; ++x ){
			for( unsigned int x = 0; x < img.getSize().x; ++x ){
				for ( unsigned int y = 0; y < img.getSize().y ;++y ){
					//cl_uint4 col = image[x + y * img.getSize().x];
					//unsigned int offset = 3 * (x + y * img.getSize().x / 2);
					unsigned int offset = 3 * (x + y * img.getSize().x);
                    cl_uint b = image[offset + 2];
					cl_uint g = image[offset + 1] + b;
                    cl_uint r = image[offset] + g;
					sf::Color col = sf::Color(
                            //sf::Uint8(clamp(std::pow(col.s[0] * factor * 2.8, 2)) * 255),
                            //sf::Uint8(clamp(std::pow(col.s[1] * factor * 3.5, 2)) * 255),
                            //sf::Uint8(clamp(std::pow(col.s[2] * factor * 8, 2)) * 255)) );
                            sf::Uint8(clamp(std::pow(r * factor * 2.8, gamma)) * 255),
                            sf::Uint8(clamp(std::pow(g * factor * 3.5, gamma)) * 255),
                            sf::Uint8(clamp(std::pow(b * factor * 8, gamma)) * 255));

					img.setPixel( x, y, col );
					//img.setPixel( screen_size.x - x - 1, y, col );
				}
			}


			// Reload texture.
			texture.loadFromImage( img );

            if( samples % ( update_frequency * numSquares ) == 0) {
                //img.saveToFile( "buddhabrot/buddhabrot_" + std::to_string( depth ) + "_" + std::to_string(square) +  ".png" );

                sf::Image sub_image;
                sub_image.create( tile_size, tile_size);

                for( unsigned int x = 0; x < tile_lines; ++x ){
                    for ( unsigned int y = 0; y < tile_lines; ++y ) {
                        unsigned int sub_square = (square % lines) * tile_lines + (square / lines) * lines * tile_lines * tile_lines;
                        sub_square += y + x * tile_lines * lines;

                        sub_image.copy(img, 0, 0, sf::IntRect( x * tile_size, y * tile_size, tile_size, tile_size));
                        sub_image.saveToFile( "buddhabrot/buddhabrot_" + std::to_string( depth + additional_depth ) + "_" + std::to_string( sub_square ) +  ".png" );
                    }
                }

                square++;

                // divide by two to skip mirror squares
                if( square >= numSquares / 2 ){
                    break;
                    depth++;
                    square = 0;
                }

                samples = 0;

                std::fill(image.begin(), image.end(), 0);
                queue.enqueueWriteBuffer( clImage, CL_TRUE, 0, image.size() * sizeof( cl_uint ), image.data() );
                err = kernel.setArg( 0, clImage);
                checkErr( err, "Kernel::setArg()" );
            }
		}

		// Redraw window.
		window.clear();

		window.draw( sprite );

		window.display();
	}

	// Create bigger tiles from lower levels

    sf::Image new_image;
    new_image.create( tile_size, tile_size);
    for ( int cur_depth = depth + additional_depth - 1; cur_depth >= 1; --cur_depth ) {
        int lines = 1 << cur_depth;
        
        for ( unsigned int xi = 0; xi < lines / 2; ++xi ) {
            for ( unsigned int yi = 0; yi < lines; ++yi ) {
                unsigned int square = yi + lines * xi;
                std::cout << cur_depth << " " << yi << " " << xi << " " << square << std::endl;
                sf::Image old_image;

                for ( unsigned int xs = 0; xs < 2; ++xs ) {
                    for ( unsigned int ys = 0; ys < 2; ++ys ) {
                        int sub_square = ( 2 * yi + ys ) + ( 2 * xi + xs) * 2 * lines;
                        old_image.loadFromFile("buddhabrot/buddhabrot_" + std::to_string( cur_depth + 1 ) + "_" + std::to_string( sub_square ) +  ".png" );

                        for ( unsigned int x = 0; x < tile_size / 2; ++x ) {
                            for ( unsigned int y = 0; y < tile_size / 2; ++y ) {
                                sf::Color col1 = old_image.getPixel( 2*x, 2*y );
                                sf::Color col2 = old_image.getPixel( 2*x, 2*y + 1 );
                                sf::Color col3 = old_image.getPixel( 2*x + 1, 2*y );
                                sf::Color col4 = old_image.getPixel( 2*x + 1, 2*y + 1 );

                                sf::Color col( static_cast<sf::Uint8>(((int)col1.r + col2.r + col3.r + col4.r ) / 4),
                                               static_cast<sf::Uint8>(((int)col1.g + col2.g + col3.g + col4.g ) / 4),
                                               static_cast<sf::Uint8>(((int)col1.b + col2.b + col3.b + col4.b ) / 4));

                                new_image.setPixel( x + xs * tile_size / 2 , y + ys * tile_size / 2, col );
                            }
                        }
                    }
                }

                new_image.saveToFile("buddhabrot/buddhabrot_" + std::to_string( cur_depth ) + "_" + std::to_string( square ) +  ".png" );
            }
        }
    }

	return 0;
}
