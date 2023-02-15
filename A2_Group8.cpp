//***********************
//Group Number : 8
//Group Member 1 : Ege Kurt
//Group Member 1 ID : 154736193
//Group Member 2 : Ching - An Shih
//Group Member 2 ID : 148221195
//Group Member 3 : Dhruvil Nareshkumar Patel
//Group Member 3 ID : 119318202
//Date : 2021 - 11 - 28
//Purpose : Assignment 2
//***********************


#include <iostream>
#include <occi.h>
#define MAX_ITEMS 5

using oracle::occi::Environment;
using oracle::occi::Connection;

using namespace oracle::occi;
using namespace std;

struct ShoppingCart {
    int product_id;
    double price;
    int quantity;
};

void Startup(Connection* conn, struct ShoppingCart* cart);
int mainMenu(void);
int customerLogin(Connection* conn, int customerId);
int addToCart(Connection* conn, struct ShoppingCart cart[]);
double findProduct(Connection* conn, int product_id);
void displayProducts(struct ShoppingCart cart[], int productCount);
int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount);
void clearStandardInputBuffer(void);

int main(void)
{
    Environment* env = nullptr;
    Connection* conn = nullptr;
    Statement* stmt = nullptr;
    ResultSet* rs = nullptr;
    struct ShoppingCart cart[MAX_ITEMS] = { {0} };

    int num;
    string str;
    string user = "dbs311_213d35";
    string pass = "10154160";
    string constr = "myoracle12c.senecacollege.ca:1521/oracle12c";

    try {

        env = Environment::createEnvironment(Environment::DEFAULT);
        conn = env->createConnection(user, pass, constr);
        //std::cout << "connection is successful" << endl;

        Statement* stmt = conn->createStatement();

        stmt->execute("CREATE OR REPLACE PROCEDURE find_customer (p_customer_id IN NUMBER, found OUT NUMBER) IS"
            " v_custid NUMBER;"
            " BEGIN"
            " found:=1;"
            " SELECT customer_id"
            " INTO v_custid"
            " FROM customers"
            " WHERE customer_id=p_customer_id;"
            " EXCEPTION"
            " WHEN NO_DATA_FOUND THEN"
            " found:=0;"
            " END;"
        );


        stmt->execute("CREATE OR REPLACE PROCEDURE find_product(p_product_id IN NUMBER, price OUT products.list_price%TYPE) IS"
            " BEGIN"
            " SELECT list_price INTO price"
            " FROM products"
            " WHERE product_id=p_product_id;"
            " EXCEPTION"
            " WHEN NO_DATA_FOUND THEN"
            " price:=0;"
            " END;"
        );

        stmt->execute("CREATE OR REPLACE PROCEDURE add_order(p_customer_id IN NUMBER, new_order_id OUT NUMBER) IS"
            " BEGIN"
            " SELECT MAX(order_id) INTO new_order_id"
            " FROM orders;"
            " new_order_id:=new_order_id+1;"
            " INSERT INTO orders"
            " VALUES(new_order_id, p_customer_id, 'Shipped', 56, sysdate);"
            " END;"
        );
        
        stmt->execute("CREATE OR REPLACE PROCEDURE"
            " add_order_item(orderId IN order_items.order_id % type,"
            " itemId IN order_items.item_id % type,"
            " productId IN order_items.product_id % type,"
            " quantity IN order_items.quantity % type,"
            " price IN order_items.unit_price % type)"
            " IS"
            " BEGIN"
            " INSERT INTO order_items"
            " VALUES(orderId, itemId, productId, quantity, price);"
            " END;"
        );

        
        //Start
        Startup(conn, cart);

        conn->terminateStatement(stmt);
        env->terminateConnection(conn);
        Environment::terminateEnvironment(env);
    }
    catch (SQLException& sqlExcp) {
        cout << "error";
        cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
    }

    return 0;
}

void Startup(Connection* conn, struct ShoppingCart* cart) {
    int option = -1, customerid = -1, isExist = 0, productcount = 0;

    do
    {
        option = mainMenu();
        switch (option)
        {
        case 0:
            cout << "Good bye!..." << endl;
            break;
        case 1:
            cout << "Enter the customer ID: ";
            cin >> customerid;
            clearStandardInputBuffer();

            isExist = customerLogin(conn, customerid);
            if (isExist) {
                productcount = addToCart(conn, cart);
                displayProducts(cart, productcount);
                checkout(conn, cart, customerid, productcount);
            }
            break;
        default:
            break;
        }
    } while (option);
   
}

int mainMenu(void) {
    int option = -1;

    cout << "\n******************** Main Menu ********************" << endl;
    cout << "1)      Login" << endl;
    cout << "0)      Exit" << endl;

    do
    {
        cout << "Enter an option (0-1) : " ;
        cin >> option;
        clearStandardInputBuffer();
        if (option < 0 || option > 1)
        {
            cout << "You entered a wrong value. ";
        }
    } while (option < 0 || option > 1);

    return option;
}

int customerLogin(Connection* conn, int customerId) {
    int customerid, found=0;

    try
    {
        Statement* stmt = conn->createStatement();
        stmt->setSQL("BEGIN find_customer(:1, :2); END;");
        stmt->setInt(1, customerId);
        stmt->setInt(2, found);
        stmt->registerOutParam(2, Type::OCCIINT, sizeof(found));
        stmt->executeUpdate();
        found = stmt->getInt(2);

        if (!found)
        {
            cout << "The customer does not exist.";
            conn->terminateStatement(stmt);
            return found;
        }
        else{
            cout << "The customer exist." << endl;
            conn->terminateStatement(stmt);
            return found;
        }
    }
    catch (SQLException& sqlExcp){
        cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
    }
    
}

int addToCart(Connection* conn, struct ShoppingCart cart[]) {
    int productId, quautity, add = 1, productcount=0;
    double list_price;

    cout << "-------------- Add Products to Cart --------------" << endl;
    //Add up to 5 items
    for(int i = 0; add && i < MAX_ITEMS; i++){

        do
        {
            cout << "Enter the product ID: ";
            cin >> productId;
            clearStandardInputBuffer();
            list_price = findProduct(conn, productId);

        } while (!list_price);
        
        if (list_price > 0) {
            cart[i].product_id = productId;
            cout << "Product Price: " << list_price << endl;
            cart[i].price = list_price;
            cout << "Enter the product Quantity: ";
            cin >> quautity;
            clearStandardInputBuffer();
            cart[i].quantity = quautity;
            cout << "Enter 1 to add more products or 0 to checkout: ";
            cin >> add;
            clearStandardInputBuffer();
            productcount++;
        }
    }

    return productcount;
}

double findProduct(Connection* conn, int product_id) {
    double price=0.00;

    try
    {
        Statement* stmt = conn->createStatement();
        stmt->setSQL("BEGIN find_product(:1, :2); END;");
        stmt->setInt(1, product_id);
        stmt->setInt(2, price);
        stmt->registerOutParam(2, Type::OCCINUMBER, sizeof(price));
        stmt->executeUpdate();
        price = stmt->getNumber(2);

        if (!price)
        {
            cout << "The product does not exists. Try again..." << endl;
            conn->terminateStatement(stmt);           
        }
        else {
            conn->terminateStatement(stmt);           
        }
    }
    catch (SQLException& sqlExcp) {
        cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
    }
    return price;
}

void displayProducts(struct ShoppingCart cart[], int productCount) {
    double total = 0;

    cout << "------- Ordered Products ---------" << endl;
    for (int i = 0; i < productCount; i++)
    {
        cout << "---Item " << i + 1 << endl;
        cout << "Product ID: " << cart[i].product_id << endl;
        cout << "Price: " << cart[i].price << endl;
        cout << "Quantity: " << cart[i].quantity << endl;
        total += cart[i].price*cart[i].quantity;
    }
    cout << "----------------------------------" << endl;
    cout << "Total: " << total << endl;
}

int checkout(Connection* conn, struct ShoppingCart cart[], int customerId, int productCount) {
    int temp, new_order_id = 0;
    char answer;

    do
    {
        temp = 1;
        cout << "Would you like to checkout? (Y/y or N/n) ";
        cin >> answer;
        clearStandardInputBuffer();
        if (answer == 'Y' || answer == 'y' || answer == 'N' || answer == 'n')
        {
            temp = 0;
        }
        else
        {
            cout << "Wrong input. Try again..." << endl;
        }
    } while (temp);

    if (answer == 'Y' || answer == 'y') {

        cout << "The order is successfully completed.";
        try
        {
            Statement* stmt = conn->createStatement();
            Statement* stmt1 = conn->createStatement();
            //Get new order id
            stmt->setSQL("BEGIN add_order(:1, :2);END;");
            stmt->setNumber(1, customerId);
            stmt->setNumber(2, new_order_id);
            stmt->registerOutParam(2, Type::OCCINUMBER, sizeof(new_order_id));
            stmt->executeUpdate();
            new_order_id = stmt->getNumber(2);
            conn->terminateStatement(stmt);

            //Add order items
            for (int i = 0; i < productCount; i++)
            {
                stmt1->setSQL("BEGIN add_order_item(:1, :2, :3, :4, :5);END;");
                stmt1->setNumber(1, new_order_id);
                stmt1->setNumber(2, i+1);
                stmt1->setNumber(3, cart[i].product_id);
                stmt1->setNumber(4, cart[i].quantity);
                stmt1->setNumber(5, cart[i].price);
                stmt1->executeUpdate();
            }        
            conn->terminateStatement(stmt1);           
        }
        catch (SQLException& sqlExcp) {
            cout << sqlExcp.getErrorCode() << ": " << sqlExcp.getMessage();
        }
        
    }
    else
    {
        cout << "The order is cancelled.";     
    }
    return 0;
}

void clearStandardInputBuffer(void)
{
    while (getchar() != '\n')
    {
        ; // On purpose: do nothing
    }
}